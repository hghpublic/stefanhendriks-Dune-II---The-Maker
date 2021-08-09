#include "../include/d2tmh.h"
#include "cBuildingListItem.h"


/**
 *
 * @param type
 * @param buildId (id of the TYPE, ie structureType ID or unitType ID or upgradeType ID)
 * @param cost
 * @param icon
 * @param totalBuildTime
 * @param list
 * @param subList
 */
cBuildingListItem::cBuildingListItem(eBuildType type, int buildId, int cost, int icon, int totalBuildTime, cBuildingList *list, int subList, bool queuable) {
    assert(buildId >= 0);
    this->buildId = buildId;
    this->cost = cost;
    this->icon = icon;
    this->totalBuildTime = totalBuildTime;
    this->type = type;
    this->queuable = queuable;
    timerCap = 0;
    progress = 0;
    buildFrameToDraw = 0;
    state = AVAILABLE;
    building = false;
    myList = list; // this can be nullptr! (it will be set from the outside by cBuildingList convenience methods)
    timesToBuild = 0;
    timesOrdered  = 0;
    slotId = -1; // is set later
    this->subList = subList;
    if (cost > 0 && totalBuildTime > 0) {
        creditsPerProgressTime = (float)this->cost / (float)this->totalBuildTime;
    }
    placeIt = false;
    deployIt = false;
    TIMER_progressFrame = 0.0f;
}

cBuildingListItem::~cBuildingListItem() {
    myList = NULL;
}

/**
 * Constructor for Structures
 * @param theID
 * @param entry
 * @param list
 * @param subList
 */
cBuildingListItem::cBuildingListItem(int theID, s_Structures entry, cBuildingList *list, int subList) :
                    cBuildingListItem(STRUCTURE, theID, entry.cost, entry.icon, entry.build_time, list, subList, entry.queuable) {
}

/**
 * Constructor for Specials
 * @param theID
 * @param entry
 * @param list
 * @param subList
 */
cBuildingListItem::cBuildingListItem(int theID, s_Special entry, cBuildingList *list, int subList) :
                    cBuildingListItem(SPECIAL, theID, 0, entry.icon, entry.buildTime, list, subList, false) {
}

/**
 * Constructor for Upgrades
 * @param theID
 * @param entry
 * @param list
 * @param subList
 */
cBuildingListItem::cBuildingListItem(int theID, s_Upgrade entry, cBuildingList *list, int subList) :
                    cBuildingListItem(UPGRADE, theID, entry.cost, entry.icon, entry.buildTime, list, subList, false) {
}

cBuildingListItem::cBuildingListItem(int theID, s_Structures entry, int subList) : cBuildingListItem(theID, entry, nullptr, subList) {
}

cBuildingListItem::cBuildingListItem(int theID, s_Special entry, int subList) : cBuildingListItem(theID, entry, nullptr, subList) {
}

/**
 * Constructor for units
 * @param theID
 * @param entry
 * @param list
 * @param subList
 */
cBuildingListItem::cBuildingListItem(int theID, s_UnitP entry, cBuildingList *list, int subList) :
                    cBuildingListItem(UNIT, theID, entry.cost, entry.icon, entry.build_time, list, subList, entry.queuable) {
}

cBuildingListItem::cBuildingListItem(int theID, s_UnitP entry, int subList) : cBuildingListItem(theID, entry, nullptr, subList) {
}

cBuildingListItem::cBuildingListItem(int theID, s_Upgrade entry, int subList) : cBuildingListItem(theID, entry, nullptr, subList) {
}

int cBuildingListItem::getCosts() {
    int costs = 0;

    if (type == UNIT) {
        costs = unitInfo[buildId].cost;
    } else if (type == STRUCTURE) {
        costs = structures[buildId].cost;
    } else if (type == UPGRADE) {
        costs = upgrades[buildId].cost;
    }
    return costs;
}

/**
 * Return the amount of money the player gets back
 * @return
 */
float cBuildingListItem::getRefundAmount() {
	float fProgress = progress;
	if (fProgress < 1.0F) {
		return 0.0F;
	}
	return (fProgress * creditsPerProgressTime);
}

void cBuildingListItem::decreaseTimesToBuild() {
	timesToBuild--;
}

void cBuildingListItem::increaseProgress(int byAmount) {
    setProgress(getProgress() + byAmount);
}

/**
 * Duplicated by getTotalBuildtime!?
 * @return
 */
int cBuildingListItem::getBuildTime() {
//    if (DEBUGGING) return 1;
    if (type == STRUCTURE) {
        return structures[buildId].build_time;
    }
    if (type == UPGRADE) {
        return upgrades[buildId].buildTime;
    }
    if (type == SPECIAL) {
        return specials[buildId].buildTime;
    }
    // assumes units by default
    return unitInfo[buildId].build_time;
}

bool cBuildingListItem::isDoneBuilding() {
    return getProgress() >= getBuildTime();
}

bool cBuildingListItem::isTypeUpgrade() {
    return isType(eBuildType::UPGRADE);
}

bool cBuildingListItem::isTypeStructure() {
    return isType(eBuildType::STRUCTURE);
}

bool cBuildingListItem::isTypeUnit() {
    return isType(eBuildType::UNIT);
}

bool cBuildingListItem::isTypeSpecial() {
    return isType(eBuildType::SPECIAL);
}

bool cBuildingListItem::isType(eBuildType value) {
    return getBuildType() == value;
}

s_Upgrade cBuildingListItem::getS_Upgrade() {
    int buildId = getBuildId();
    if (getBuildType() != eBuildType::UPGRADE){
        logbook("ERROR!!! - calling getS_Upgrade while type is not UPGRADE! - falling back to buildId 1 as safety");
        buildId = 1;
    }
    return upgrades[buildId];
}

s_Special cBuildingListItem::getS_Special() {
    int buildId = getBuildId();
    if (getBuildType() != eBuildType::SPECIAL){
        logbook("ERROR!!! - calling gets_Special while type is not SPECIAL! - falling back to buildId 1 as safety");
        buildId = 1;
    }
    return specials[buildId];
}

s_UnitP cBuildingListItem::getS_UnitP() {
    int buildId = getBuildId();
    if (getBuildType() != eBuildType::UNIT){
        logbook("ERROR!!! - calling getS_UnitP while type is not UNIT! - falling back to buildId 1 as safety");
        buildId = 1;
    }
    return unitInfo[buildId];
}

s_Structures cBuildingListItem::getS_Structures() {
    int buildId = getBuildId();
    if (getBuildType() != eBuildType::STRUCTURE){
        logbook("ERROR!!! - calling getS_Structures while type is not STRUCTURE! - falling back to buildId 1 as safety");
        buildId = 1;
    }
    return structures[buildId];
}

void cBuildingListItem::resetTimesOrdered() {
    timesOrdered = 0;
}

eListType cBuildingListItem::getListType() {
    if (myList) {
        return myList->getType();
    }
    return eListType::LIST_NONE;
}

int cBuildingListItem::calculateBuildProgressFrameBasedOnBuildProgress() {
    // frame to draw (for building in progress)
    int iFrame = health_bar(31, progress, getTotalBuildTime());

    if (iFrame > 31) {
        iFrame = 31;
    }

    return iFrame;
}

void cBuildingListItem::decreaseProgressFrameTimer() {
    TIMER_progressFrame--;
}

float cBuildingListItem::getProgressFrameTimer() {
    return TIMER_progressFrame;
}

void cBuildingListItem::resetProgressFrameTimer() {
    // total time to build is progress * 35 (time unit).
    // divide that by frames (31), and get the time between frames!
    int timeSpent = timerCap * totalBuildTime;
    if (timeSpent > 0) {
        TIMER_progressFrame = timeSpent / 31;
    } else {
        TIMER_progressFrame = 0;
    }
}

void cBuildingListItem::setTimerCap(int value) {
    timerCap = value;
}
