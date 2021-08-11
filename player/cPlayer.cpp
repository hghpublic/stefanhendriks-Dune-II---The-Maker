#include <vector>
#include <algorithm>
#include "../include/d2tmh.h"
#include "cPlayer.h"


cPlayer::cPlayer() {
    itemBuilder = NULL;
    orderProcesser = NULL;
    sidebar = NULL;
    buildingListUpdater = NULL;
    gameControlsContext = NULL;
    char msg[255];
    sprintf(msg, "MAX_STRUCTURETYPES=[%d], sizeof bmp_structure=%d, sizeof(BITMAP *)", MAX_STRUCTURETYPES,
            sizeof(bmp_structure), sizeof(BITMAP *));
    logbook(msg); // no log(), because we cant assume player is fully initialized yet
    memset(bmp_structure, 0, sizeof(bmp_structure));
    memset(bmp_unit, 0, sizeof(bmp_unit));
    memset(bmp_unit_top, 0, sizeof(bmp_unit_top));
    brain_ = nullptr;
    autoSlabStructures = false;
}

cPlayer::~cPlayer() {
    if (itemBuilder) {
        delete itemBuilder;
    }
    if (orderProcesser) {
        delete orderProcesser;
    }
    if (sidebar) {
        delete sidebar;
    }
    if (buildingListUpdater) {
        delete buildingListUpdater;
    }
    if (gameControlsContext) {
        delete gameControlsContext;
    }
    if (difficultySettings) {
        delete difficultySettings;
    }
    if (brain_) {
        delete brain_;
    }
    // cannot do this in destructor, as Allegro is already shutdown
//    destroyAllegroBitmaps();
}

void cPlayer::destroyAllegroBitmaps() {
    clearStructureTypeBitmaps();
    clearUnitTypeBitmaps();
}

void cPlayer::clearStructureTypeBitmaps() {
    for (int i = 0; i < MAX_STRUCTURETYPES; i++) {
        if (bmp_structure[i]) {
//            char msg[255];
//            sprintf(msg, "clearStructureTypeBitmaps: Destroying bmp_structure for index [%d].", i);
//            log(msg);
            destroy_bitmap(bmp_structure[i]);
        } else {
//            char msg[255];
//            sprintf(msg, "clearStructureTypeBitmaps: Index [%d] is null.", i);
//            log(msg);
        }
        bmp_structure[i] = nullptr;
    }
}

void cPlayer::setFocusCell(int cll) {
    this->focusCell_ = cll;
}

void cPlayer::clearUnitTypeBitmaps() {
    for (int i = 0; i < MAX_UNITTYPES; i++) {
        if (bmp_unit[i]) {
            if (DEBUGGING) {
//                char msg[255];
//                sprintf(msg, "clearUnitTypeBitmaps: Destroying bmp_unit for index [%d].", i);
//                log(msg);
            }
            destroy_bitmap(bmp_unit[i]);
        }
        bmp_unit[i] = nullptr;

        if (bmp_unit_top[i]) {
            if (DEBUGGING) {
//                char msg[255];
//                sprintf(msg, "clearUnitTypeBitmaps: Destroying bmp_unit_top for index [%d].", i);
//                log(msg);
            }
            destroy_bitmap(bmp_unit_top[i]);
        }
        bmp_unit_top[i] = nullptr;
    }
}

void cPlayer::setSideBar(cSideBar *theSideBar) {
    assert(theSideBar);

    // delete old reference
    if (sidebar) {
        delete sidebar;
    }

    sidebar = theSideBar;
}

void cPlayer::setItemBuilder(cItemBuilder *theItemBuilder) {
    assert(theItemBuilder);

    // delete old reference
    if (itemBuilder) {
        delete itemBuilder;
    }

    itemBuilder = theItemBuilder;
}

void cPlayer::setOrderProcesser(cOrderProcesser *theOrderProcesser) {
    assert(theOrderProcesser);

    if (orderProcesser) {
        delete orderProcesser;
    }

    orderProcesser = theOrderProcesser;
}

void cPlayer::setBuildingListUpdater(cBuildingListUpdater *theBuildingListUpgrader) {
    assert(theBuildingListUpgrader);

    // delete old reference
    if (buildingListUpdater) {
        delete buildingListUpdater;
    }

    buildingListUpdater = theBuildingListUpgrader;
}

void cPlayer::setGameControlsContext(cGameControlsContext *theGameControlsContext) {
    assert(theGameControlsContext);

    // delete old reference
    if (gameControlsContext) {
        delete gameControlsContext;
    }

    gameControlsContext = theGameControlsContext;
}

void cPlayer::init(int id, brains::cPlayerBrain *brain) {
    if (id < 0 || id >= MAX_PLAYERS) {
        char msg[255];
        sprintf(msg, "Error initializing player, id %d is not valid.", id);
        logbook(msg); // no log(), as house still has to be set up
    }
    assert(id >= HUMAN);
    assert(id < MAX_PLAYERS);
    this->id = id;

    setBrain(brain);

    memcpy(pal, general_palette, sizeof(pal));
    house = GENERALHOUSE;

    /**
     * Ok, so this is confusing.
     * There are also aiPlayer classes. They hold some 'brains' I guess. all other state is stored here.
     * So even though there are aiPlayer objects, they are complimentary to this class
     */
    m_Human = (id == HUMAN);

    if (difficultySettings) delete difficultySettings;
    difficultySettings = new cPlayerDifficultySettings();

    // Reset structures amount
    for (int i = 0; i < MAX_STRUCTURETYPES; i++) {
        iStructures[i] = 0;
        iPrimaryBuilding[i] = -1;
        iStructureUpgradeLevel[i] = 0;
    }

    credits = 0;
    maxCredits_ = 1000;
    focusCell_ = 0;

    powerUsage_ = 0;
    powerProduce_ = 0;

    iTeam = -1;
}

/**
 * Sets house value , but also sets up difficulty settings + creates bitmaps for that corresponding house
 * @param iHouse
 */
void cPlayer::setHouse(int iHouse) {
    int currentHouse = house;
    char msg[255];
    sprintf(msg, "cPlayer[%d]::setHouse - Current house is [%d/%s], setting house to [%d/%s]", this->id, currentHouse,
            this->getHouseNameForId(currentHouse).c_str(), iHouse, this->getHouseNameForId(iHouse).c_str());
    logbook(msg); // not yet set up house properly.. so use logbook instead of log()
    house = iHouse;      // use rules of this house

    if (difficultySettings) {
        delete difficultySettings;
    }

    difficultySettings = cPlayerDifficultySettings::createFromHouse(iHouse);

    if (currentHouse != iHouse) {
        // copy entire palette
        memcpy(pal, general_palette, sizeof(pal));

        char msg[255];
        sprintf(msg, "cPlayer[%d]::setHouse - Current house differs from iHouse, preparing palette.", this->id);
        logbook(msg);

        // now set the different colors based upon house
        if (houses[house].swap_color > -1) {
            int start = houses[house].swap_color;
            int s = 144;                // original position (harkonnen)
            char msg[255];
            sprintf(msg, "cPlayer[%d]::setHouse - Swap_color index is %d.", this->id, start);
            logbook(msg);
            for (int j = start; j < (start + 7); j++) {
                // swap everything from S with J
                pal[s] = pal[j];
                s++;
            }
        }

        minimapColor = houses[house].minimap_color;
        emblemBackgroundColor = getEmblemBackgroundColorForHouse(house);

        destroyAllegroBitmaps();

        int colorDepthBmpScreen = bitmap_color_depth(bmp_screen);

        // use this palette to draw stuff
        select_palette(pal);

        // now copy / set all structures for this player, with the correct color
        for (int i = 0; i < MAX_STRUCTURETYPES; i++) {
            s_Structures &structureType = structures[i];

            if (!structureType.configured) continue;

            bmp_structure[i] = create_bitmap_ex(colorDepthBmpScreen, structureType.bmp->w, structureType.bmp->h);
            if (!bmp_structure[i]) {
                allegro_message("Could not create bmp structure bitmap!? - Imminent crash.");
            }
            clear_to_color(bmp_structure[i], makecol(255, 0, 255));

            draw_sprite(bmp_structure[i], structureType.bmp, 0, 0);
        }

        // same goes for units
        for (int i = 0; i < MAX_UNITTYPES; i++) {
            s_UnitP &unitType = unitInfo[i];

            bmp_unit[i] = create_bitmap_ex(colorDepthBmpScreen, unitType.bmp->w, unitType.bmp->h);
            if (!bmp_unit[i]) {
                allegro_message("Could not create bmp unit bitmap!? - Imminent crash.");
            }
            clear_to_color(bmp_unit[i], makecol(255, 0, 255));

            draw_sprite(bmp_unit[i], unitType.bmp, 0, 0);

            if (unitType.top) {
                bmp_unit_top[i] = create_bitmap_ex(colorDepthBmpScreen, unitType.bmp->w, unitType.bmp->h);
                clear_to_color(bmp_unit_top[i], makecol(255, 0, 255));

                draw_sprite(bmp_unit_top[i], unitType.top, 0, 0);
            }
        }
    }
}

int cPlayer::getEmblemBackgroundColorForHouse(int houseId) {
    switch (houseId) {
        case ATREIDES:
            return makecol(8, 12, 89);
        case HARKONNEN:
            return makecol(60, 0, 0);
        case ORDOS:
            return makecol(0, 32, 0);
        case SARDAUKAR:
            return makecol(128, 0, 128);
        default:
            return makecol(100, 255, 100);
    }
}

bool cPlayer::hasAlmostReachMaxSpiceStorageCapacity() const {
    return !bEnoughSpiceCapacityToStoreCredits(250);
}

bool cPlayer::bEnoughSpiceCapacityToStoreCredits() const {
    return bEnoughSpiceCapacityToStoreCredits(0);
}

bool cPlayer::bEnoughSpiceCapacityToStoreCredits(int threshold) const {
    return (maxCredits_ - threshold) > credits;
}

bool cPlayer::bEnoughPower() const {
    if (!game.bSkirmish) {
        // AI cheats on power
        if (!m_Human) {
            // Dune 2 non-skirmish AI cheats; else it will be unplayable in some missions.
            // the Dune 2 AI basically has always enough power, to make it somewhat more 'fair' lets check for
            // ConstYard + Windtrap. If both destroyed we are out of power.
            return hasAtleastOneStructure(WINDTRAP) || hasAtleastOneStructure(CONSTYARD);
        }
    }

    return powerProduce_ >= powerUsage_;
}

bool cPlayer::hasRadarAndEnoughPower() const {
    return getAmountOfStructuresForType(RADAR) > 0 && bEnoughPower();
}

/**
 * This function returns the amount for the given structure type. If structureType param is invalid, then it will
 * return -1
 * @param structureType
 * @return
 */
int cPlayer::getAmountOfStructuresForType(int structureType) const {
    if (structureType < 0 || structureType > RTURRET) return -1;
    return iStructures[structureType];
}

/**
 * This function will return the amount of units for given type, but it is not (yet) optimized, so it will
 * loop over all units and count them. Use it with caution.
 * return -1
 * @param unitType
 * @return
 */
int cPlayer::getAmountOfUnitsForType(int unitType) const {
    if (unitType < 0 || unitType > MAX_UNITTYPES) return -1;
    int count = 0;
    for (int i = 0; i < MAX_UNITS; i++) {
        cUnit &cUnit = unit[i];
        if (!cUnit.isValid()) continue;
        if (cUnit.iPlayer != this->getId()) continue;
        if (cUnit.iType == unitType) {
            count++;
        }
    }
    return count;
}

/**
 * This function will return the amount of units for given type, but it is not (yet) optimized, so it will
 * loop over all units and count them. Use it with caution.
 * return -1
 * @param unitTypes (vector of all unitTypes to check)
 * @return
 */
int cPlayer::getAmountOfUnitsForType(std::vector<int> unitTypes) const {
    int count = 0;
    for (int i = 0; i < MAX_UNITS; i++) {
        cUnit &cUnit = unit[i];
        if (!cUnit.isValid()) continue;
        if (cUnit.iPlayer != this->getId()) continue;
        if (std::find(unitTypes.begin(), unitTypes.end(), cUnit.iType) != unitTypes.end()) {
            count++;
        }
    }
    return count;
}


/**
 * Returns the bitmap for structure type "index", this structure has been colorized beforehand for this player and is
 * in same color depth as bmp_screen.
 * @param index
 * @return
 */
BITMAP *cPlayer::getStructureBitmap(int index) {
    if (bmp_structure[index]) {
        return bmp_structure[index];
    }
    return nullptr;
}

/**
 * Returns the bitmap for unit type "index", this unit has been colorized beforehand for this player and is
 * in same color depth as bmp_screen.
 * @param index
 * @return
 */
BITMAP *cPlayer::getUnitBitmap(int index) {
    if (bmp_unit[index]) {
        return bmp_unit[index];
    }
    return nullptr;
}

/**
 * Returns the bitmap for unit top type "index", this top of unit has been colorized beforehand for this player and is
 * in same color depth as bmp_screen.
 * @param index
 * @return
 */
BITMAP *cPlayer::getUnitTopBitmap(int index) {
    if (bmp_unit_top[index]) {
        return bmp_unit_top[index];
    }
    return nullptr;
}

/**
 * Returns the shadow bitmap of unit type "index", using bodyFacing and animationFrame.
 * !!! Be sure to destroy the bitmap returned from here !!!
 * @param index
 * @return
 */
BITMAP *cPlayer::getUnitShadowBitmap(int index, int bodyFacing, int animationFrame) {
    if (unitInfo[index].shadow) {
        int bmp_width = unitInfo[index].bmp_width;
        int bmp_height = unitInfo[index].bmp_height;
        int start_x = bodyFacing * bmp_width;
        int start_y = bmp_height * animationFrame;

        // Carry-all has a bit different offset for shadow
        if (index == CARRYALL) {
            start_x += 2;
            start_y += 2;
        }

        int colorDepth = bitmap_color_depth(bmp_screen);
        BITMAP *shadow = create_bitmap_ex(colorDepth, bmp_width, bmp_height);
        clear_to_color(shadow, makecol(255, 0, 255));

        blit((BITMAP *) unitInfo[index].shadow, shadow, start_x, start_y, 0, 0, bmp_width, bmp_height);
        return shadow;
    }
    return nullptr;
}

bool cPlayer::hasWor() const {
    return iStructures[WOR] > 0;
}

bool cPlayer::hasBarracks() const {
    return iStructures[BARRACKS] > 0;
}

bool cPlayer::hasAtleastOneStructure(int structureType) const {
    if (structureType < 0) return false;
    if (structureType >= MAX_STRUCTURETYPES) return false;

    return iStructures[structureType] > 0;
}

bool cPlayer::hasEnoughCreditsFor(float requestedAmount) const {
    return credits > requestedAmount;
}

bool cPlayer::hasEnoughCreditsForUnit(int unitType) {
    if (unitType < 0 || unitType >= MAX_UNITTYPES) return false;
    return this->credits >= unitInfo[unitType].cost;
}

bool cPlayer::hasEnoughCreditsForStructure(int structureType) {
    if (structureType < 0 || structureType >= MAX_STRUCTURETYPES) return false;
    return this->credits >= structures[structureType].cost;
}

bool cPlayer::hasEnoughCreditsForUpgrade(int upgradeType) {
    if (upgradeType < 0 || upgradeType >= MAX_UPGRADETYPES) return false;
    return this->credits >= upgrades[upgradeType].cost;
}

/**
 * Returns house based fading/pulsating color
 * @return
 */
int cPlayer::getHouseFadingColor() const {
    int fadeSelect = game.getFadeSelect();
    if (house == ATREIDES) {
        return makecol(0, 0, fadeSelect);
    }
    if (house == HARKONNEN) {
        return makecol(fadeSelect, 0, 0);
    }
    if (house == ORDOS) {
        return makecol(0, fadeSelect, 0);
    }

    // TODO other houses (Sardaukar, etc)
    return makecol(fadeSelect, fadeSelect, fadeSelect);
}

/**
 * Returns the error fading color (red to black pulsating)
 * @return
 */
int cPlayer::getErrorFadingColor() const {
    int fadeSelect = game.getFadeSelect();
    return makecol(fadeSelect, 0, 0); // red fading
}

/**
 * Returns the error fading color (red to black pulsating)
 * @return
 */
int cPlayer::getPrimaryBuildingFadingColor() const {
    int fadeSelect = game.getFadeSelect();
    return makecol(0, fadeSelect, 0); // green fading
}

/**
 * Returns the fading white color
 * @return
 */
int cPlayer::getSelectFadingColor() const {
    int fadeSelect = game.getFadeSelect();
    return makecol(fadeSelect, fadeSelect, fadeSelect); // white fading
}

eHouseBitFlag cPlayer::getHouseBitFlag() {
    switch (house) {
        case ATREIDES:
            return eHouseBitFlag::Atreides;
        case HARKONNEN:
            return eHouseBitFlag::Harkonnen;
        case ORDOS:
            return eHouseBitFlag::Ordos;
        case SARDAUKAR:
            return eHouseBitFlag::Sardaukar;
        case FREMEN:
            return eHouseBitFlag::Fremen;
        default:
            return eHouseBitFlag::Unknown;
    }
}

void cPlayer::increaseStructureAmount(int structureType) {
    if (structureType < 0) return;
    if (structureType >= MAX_STRUCTURETYPES) return;
    iStructures[structureType]++;

    char msg[255];
    sprintf(msg, "increaseStructureAmount result: iStructures[%d(=%s)]=%d", structureType,
            structures[structureType].name, iStructures[structureType]);
    log(msg);
}

void cPlayer::decreaseStructureAmount(int structureType) {
    if (structureType < 0) return;
    if (structureType >= MAX_STRUCTURETYPES) return;

    iStructures[structureType]--;

    char msg[255];
    sprintf(msg, "decreaseStructureAmount result: iStructures[%d(=%s)]=%d", structureType,
            structures[structureType].name, iStructures[structureType]);
    log(msg);
}

std::string cPlayer::getHouseName() const {
    return getHouseNameForId(house);
}

std::string cPlayer::getHouseNameForId(int house) {
    if (house == ATREIDES) {
        return "Atreides";
    } else if (house == HARKONNEN) {
        return "Harkonnen";
    } else if (house == ORDOS) {
        return "Ordos";
    } else if (house == SARDAUKAR) {
        return "Sardaukar";
    } else if (house == FREMEN) {
        return "Fremen";
    } else if (house == MERCENARY) {
        return "Mercenary";
    } else if (house == GENERALHOUSE) {
        return "Generalhouse (none)";
    } else if (house == CORRINO) {
        return "Corrino / House for worms";
    }
    return "Unknown !?";
}

void cPlayer::giveCredits(float amountToGive) {
    credits += amountToGive;
}

// TODO: This could be done smarter once we receive notifications when a unit gets created/destroyed!
std::vector<int> cPlayer::getAllMyUnits() {
    std::vector<int> ids = std::vector<int>();
    for (int i = 0; i < MAX_UNITTYPES; i++) {
        cUnit &cUnit = unit[i];
        if (!cUnit.isValid()) continue;
        if (cUnit.isDead()) continue;
        if (cUnit.iPlayer != getId()) continue;
        ids.push_back(i);
    }
    return ids;
}

// TODO: This can be done smarter because we receive notifications when a structure gets created/destroyed!
std::vector<int> cPlayer::getAllMyStructuresAsId() {
    std::vector<int> ids = std::vector<int>();
    for (int i = 0; i < MAX_STRUCTURES; i++) {
        cAbstractStructure *abstractStructure = structure[i];
        if (!abstractStructure) continue;
        if (!abstractStructure->isValid()) continue;
        if (!abstractStructure->belongsTo(this)) continue;
        ids.push_back(i);
    }
    return ids;
}

bool cPlayer::isSameTeamAs(cPlayer *pPlayer) {
    if (pPlayer == nullptr) return false;
    return pPlayer->iTeam == iTeam;
}

/**
 * Update player state
 */
void cPlayer::update() {
    powerUsage_ = structureUtils.getTotalPowerUsageForPlayer(this);
    powerProduce_ = structureUtils.getTotalPowerOutForPlayer(this);
    // update spice capacity
    maxCredits_ = structureUtils.getTotalSpiceCapacityForPlayer(this);
}

int cPlayer::getCredits() {
    return credits;
}

void cPlayer::setCredits(int credits) {
    this->credits = credits;
}

void cPlayer::substractCredits(int amount) {
    credits -= amount;
}

float cPlayer::getMaxCredits() {
    return maxCredits_;
}

int cPlayer::getPowerProduced() {
    return powerProduce_;
}

int cPlayer::getPowerUsage() {
    return powerUsage_;
}

/**
 * Gives the player credits, but caps at max spice/credits capacity.
 * @param amount
 */
void cPlayer::dumpCredits(int amount) {
    giveCredits(amount);
    if (credits > maxCredits_) {
        credits = maxCredits_;
    }
}

/**
 * Think function, called every 100ms.
 */
void cPlayer::think() {
    if (brain_) {
        brain_->think();
    }
}

void cPlayer::setBrain(brains::cPlayerBrain *brain) {
    // delete old brain object if it was set before
    if (brain_) {
        delete brain_;
    }
    // set new brain
    brain_ = brain;
}

bool cPlayer::canBuildStructureType(int iStructureType) const {
    cBuildingListItem *pItem = sidebar->getBuildingListItem(LIST_CONSTYARD, iStructureType);
    return pItem != nullptr;
}

bool cPlayer::canBuildUnitType(int iUnitType) const {
    int listId = unitInfo[iUnitType].listId;
    cBuildingListItem *pItem = sidebar->getBuildingListItem(listId, iUnitType);
    char msg[255];
    bool result = pItem != nullptr;
    sprintf(msg, "canBuildUnitType(unitType=%d) -> %s", iUnitType, result ? "TRUE" : "FALSE");
    log(msg);
    return result;
}

bool cPlayer::canBuildSpecialType(int iType) const {
    int listId = specials[iType].listId;
    cBuildingListItem *pItem = sidebar->getBuildingListItem(listId, iType);

    bool result = pItem != nullptr;

    char msg[255];
    sprintf(msg, "canBuildSpecialType(iType=%d) -> %s", iType, result ? "TRUE" : "FALSE");
    log(msg);

    return result;
}

int cPlayer::getStructureTypeBeingBuilt() const {
    cBuildingListItem *pItem = getStructureBuildingListItemBeingBuilt();
    if (pItem) {
        return pItem->getBuildId();
    }
    return -1;
}

cBuildingListItem *cPlayer::getStructureBuildingListItemBeingBuilt() const {
    return itemBuilder->getListItemWhichIsBuilding(LIST_CONSTYARD, 0);
}

/**
 * Checks if anything is built in the list/sublist combination for this unitType. This could mean the unitType
 * itself is being built OR that another unit is built using the same list/sublist as this unitType. Example: A QUAD
 * and a TRIKE share the same list/sublist. If a QUAD is being built and you pass in the unitType TRIKE then this
 * function will return true.
 * @param iUnitType
 * @return
 */
bool cPlayer::isBuildingSomethingInSameListSubListAsUnitType(int iUnitType) const {
    s_UnitP &p = unitInfo[iUnitType];
    int listId = p.listId;
    int subListId = p.subListId;

    return isBuildingAnythingForListAndSublist(listId, subListId);
}

bool cPlayer::isBuildingAnythingForListAndSublist(int listId,
                                                  int subListId) const {
    return itemBuilder->isAnythingBeingBuiltForListId(listId, subListId);
}


/**
 * Checks if the given structureType is available for upgrading. If so, returns the corresponding cBuildingListItem
 * which can be used later (ie, to execute an upgrade, or check other info)
 *
 * @param iStructureType
 * @return
 */
cBuildingListItem *cPlayer::isUpgradeAvailableToGrantStructure(int iStructureType) const {
    return isUpgradeAvailableToGrant(STRUCTURE, iStructureType);
}


/**
 * Checks if the given unitType is available for upgrading. If so, returns the corresponding cBuildingListItem
 * which can be used later (ie, to execute an upgrade, or check other info)
 *
 * @param iUnitTYpe
 * @return
 */
cBuildingListItem *cPlayer::isUpgradeAvailableToGrantUnit(int iUnitType) const {
    return isUpgradeAvailableToGrant(UNIT, iUnitType);
}

/**
 * Checks if an upgrade exists for the 'providesType' (ie UNIT/STRUCTURE) and a specific kind of that type (providesTypeId), ie
 * a QUAD, or RTURRET, etc.
 *
 * @param providesType (UNIT/STRUCTURE)
 * @param providesTypeId (ex: QUAD, TANK, RTURRET, etc)
 * @return
 */
cBuildingListItem *cPlayer::isUpgradeAvailableToGrant(eBuildType providesType, int providesTypeId) const {
    cBuildingList *pList = sidebar->getList(LIST_UPGRADES);
    for (int i = 0; i < MAX_ITEMS; i++) {
        cBuildingListItem *pItem = pList->getItem(i);
        if (pItem == nullptr) continue;
        const s_Upgrade &theUpgrade = pItem->getS_Upgrade();
        if (theUpgrade.providesType != providesType) continue;
        if (theUpgrade.providesTypeId == providesTypeId) {
            return pItem;
        }
    }
    return nullptr;
}


/**
 * Checks if any Upgrade is in progress for the given listId/sublistId
 *
 * @param listId
 * @param sublistId
 * @return
 */
cBuildingListItem *cPlayer::isUpgradingList(int listId, int sublistId) const {
    cBuildingList *upgradesList = sidebar->getList(LIST_UPGRADES);
    if (upgradesList == nullptr) {
        char msg[255];
        sprintf(msg,
                "isUpgradingList for listId [%d] and sublistId [%d], could not find upgradesList!? - will return FALSE.",
                getId(), listId, sublistId);
        log(msg);
        assert(false);
        return nullptr;
    }

    for (int i = 0; i < MAX_ITEMS; i++) {
        cBuildingListItem *pItem = upgradesList->getItem(i);
        if (pItem == nullptr) continue;
        const s_Upgrade &theUpgrade = pItem->getS_Upgrade();
        // is this upgrade applicable to the listId/sublistId we're interested in?
        if (theUpgrade.providesTypeList == listId && theUpgrade.providesTypeSubList == sublistId) {
            if (pItem->isBuilding()) {
                // it is in progress, so yes!
                return pItem;
            }
        }
    }
    return nullptr;
}

cBuildingListItem *cPlayer::isUpgradingConstyard() const {
    return isUpgradingList(LIST_CONSTYARD, 0);
}

/**
 * Returns true if anything is built from ConstYard
 */
cBuildingListItem *cPlayer::isBuildingStructure() const {
    return itemBuilder->getListItemWhichIsBuilding(LIST_CONSTYARD, 0);
}

bool cPlayer::startBuildingUnit(int iUnitType) const {
    s_UnitP &unitType = unitInfo[iUnitType];
    int listId = unitType.listId;
    bool startedBuilding = sidebar->startBuildingItemIfOk(listId, iUnitType);

    if (DEBUGGING) {
        char msg[255];
        if (startedBuilding) {
            sprintf(msg, "Wanting to build unit [%s] iUnitType = [%d], with listId[%d] - SUCCESS", unitType.name,
                    iUnitType, listId);
        } else {
            sprintf(msg, "Wanting to build unit [%s] iUnitType = [%d], with listId[%d] - FAILED", unitType.name,
                    iUnitType, listId);
        }
        log(msg);
    }
    return startedBuilding;
}

bool cPlayer::startBuildingStructure(int iStructureType) const {
    int listId = LIST_CONSTYARD;

    bool startedBuilding = sidebar->startBuildingItemIfOk(listId, iStructureType);

    if (DEBUGGING) {
        char msg[255];
        if (startedBuilding) {
            sprintf(msg, "Wanting to build structure [%s] iStructureType = [%d], with listId[%d] - SUCCESS",
                    structures[iStructureType].name, iStructureType, listId);
        } else {
            sprintf(msg, "Wanting to build structure [%s] iStructureType = [%d], with listId[%d] - FAILED",
                    structures[iStructureType].name, iStructureType, listId);
        }
        log(msg);
    }
    return startedBuilding;
}

bool cPlayer::startBuildingSpecial(int iSpecialType) const {
    int listId = LIST_PALACE;

    bool startedBuilding = sidebar->startBuildingItemIfOk(listId, iSpecialType);

    if (DEBUGGING) {
        char msg[255];
        if (startedBuilding) {
            sprintf(msg, "Wanting to build special [%s] iSpecialType = [%d], with listId[%d] - SUCCESS",
                    specials[iSpecialType].description, iSpecialType, listId);
        } else {
            sprintf(msg, "Wanting to build special [%s] iSpecialType = [%d], with listId[%d] - FAILED",
                    specials[iSpecialType].description, iSpecialType, listId);
        }
        log(msg);
    }
    return startedBuilding;
}

bool cPlayer::startUpgrading(int iUpgradeType) const {
    int listId = LIST_UPGRADES;
    bool startedBuilding = sidebar->startBuildingItemIfOk(listId, iUpgradeType);

    if (DEBUGGING) {
        char msg[255];
        if (startedBuilding) {
            sprintf(msg, "Wanting to start upgrade [%s] iUpgradeType = [%d], with listId[%d] - SUCCESS",
                    upgrades[iUpgradeType].description, iUpgradeType, listId);
        } else {
            sprintf(msg, "Wanting to start upgrade [%s] iUpgradeType = [%d], with listId[%d] - FAILED",
                    upgrades[iUpgradeType].description, iUpgradeType, listId);
        }
        log(msg);
    }
    return startedBuilding;
}

/**
 * Returns true if anything is built from ConstYard
 */
bool cPlayer::isBuildingStructureAwaitingPlacement() const {
    return itemBuilder->isAnythingBeingBuiltForListIdAwaitingPlacement(LIST_CONSTYARD, 0);
}

/**
 * Evaluates where to place a structure. Returns a cell where structure can be placed. Will assume the area has
 * proper terrain, is not blocked by any other structure. If there are any friendly units there, it assumes
 * they can be moved away, and thus ignores them. However, enemy units will block placement.
 *
 * When no cell can be found, this function will return -1
 *
 * @param structureType
 * @return
 */
int cPlayer::findCellToPlaceStructure(int structureType) {
    // find place (fast, if possible), where to place it
    // ignore any units (we can move them out of the way). But do take
    // terrain and other structures into consideration!

    int centerOfBase = getFocusCell();

    const std::vector<int> &allMyStructuresAsId = getAllMyStructuresAsId();
    std::vector<int> potentialCells = std::vector<int>();

    int iWidth = structures[structureType].bmp_width / TILESIZE_WIDTH_PIXELS;
    int iHeight = structures[structureType].bmp_height / TILESIZE_HEIGHT_PIXELS;

    for (auto &id : allMyStructuresAsId) {
        cAbstractStructure * aStructure = structure[id];

        // go around any structure, and try to find a cell where we can place a structure.
        int iStartX = map.getCellX(aStructure->getCell());
        int iStartY = map.getCellY(aStructure->getCell());

        int iEndX = iStartX + aStructure->getWidth(); // not plus 1 because iStartX is 1st cell
        int iEndY = iStartY + aStructure->getHeight(); // not plus 1 because iStartY is 1st cell

        // start is topleft/above structure, but also take size of the structure to place
        // into acount. So ie, a structure of 2x2 will be attempted (at first) at y - 2.
        // attempt at 'top' first:
        int topLeftX = iStartX - iWidth;
        int topLeftY = iStartY - iHeight;

        // check: from top left to top right
        for (int sx = topLeftX; sx < iEndX; sx++) {
            int cell = map.getCellWithMapBorders(sx, topLeftY);
            if (cell < 0) continue;

            const s_PlaceResult &result = canPlaceStructureAt(cell, structureType);
            bool canPlaceStructureAt = result.success || result.onlyMyUnitsBlock;
            if (canPlaceStructureAt) {
                potentialCells.push_back(cell);
            }
        }

        int bottomLeftX = topLeftX;
        int bottomLeftY = iEndY;
        // check: from bottom left to bottom right
        for (int sx = bottomLeftX; sx < iEndX; sx++) {
            int cell = map.getCellWithMapBorders(sx, bottomLeftY);
            if (cell < 0) continue;

            const s_PlaceResult &result = canPlaceStructureAt(cell, structureType);
            bool canPlaceStructureAt = result.success || result.onlyMyUnitsBlock;
            if (canPlaceStructureAt) {
                potentialCells.push_back(cell);
            }
        }

        // left to structure (not from top!)
        int justLeftX = topLeftX;
        int justLeftY = iStartY - (iHeight - 1);
        for (int sy = justLeftY; sy < iEndY; sy++) {
            int cell = map.getCellWithMapBorders(justLeftX, sy);
            if (cell < 0) continue;

            const s_PlaceResult &result = canPlaceStructureAt(cell, structureType);
            bool canPlaceStructureAt = result.success || result.onlyMyUnitsBlock;
            if (canPlaceStructureAt) {
                potentialCells.push_back(cell);
            }
        }

        // right to structure (not top!)
        int justRightX = iEndX;
        int justRightY = iStartY - (iHeight - 1);
        for (int sy = justRightY; sy < iEndY; sy++) {
            int cell = map.getCellWithMapBorders(justRightX, sy);
            if (cell < 0) continue;

            const s_PlaceResult &result = canPlaceStructureAt(cell, structureType);
            bool canPlaceStructureAt = result.success || result.onlyMyUnitsBlock;
            if (canPlaceStructureAt) {
                potentialCells.push_back(cell);
            }
        }

        // if we have found any we randomly abort
        if (!potentialCells.empty()) {
            if (rnd(100) < 33) {
                break;
            }
        }
    }

    if (!potentialCells.empty()) {
        if (structureType == TURRET || structureType == RTURRET) {
//                // first shuffle, before going through the list
//                std::random_shuffle(potentialCells.begin(), potentialCells.end());
//
//                std::vector<int> potentialFurtherCells = std::vector<int>();
//                int found = 0;
//                double distance = 128; // arbitrary distance as 'border'
//                for (auto &potentialCell : potentialCells) {
//                    double dist = map.distance(centerOfBase, potentialCell);
//                    if (dist > distance) {
//                        potentialFurtherCells.push_back(potentialCell);
//                        found++;
//                        if (found > 5) {
//                            break;
//                        }
//                    }
//                }
//
//                if (!potentialFurtherCells.empty()) {
//                    // shuffle the 5 'furthest'
//                    std::random_shuffle(potentialFurtherCells.begin(), potentialFurtherCells.end());
//                }

            // for now pick random position, but in the future do something more smart
            std::random_shuffle(potentialCells.begin(), potentialCells.end());
        } else {
            // found one, shuffle, and then return the first
            std::random_shuffle(potentialCells.begin(), potentialCells.end());
        }
        return potentialCells.front();
    }

    return -1;
}

eCantBuildReason cPlayer::canBuildUnit(int iUnitType, bool checkIfAffordable) {
    // Once known, a check will be made to see if the AI has a structure to produce that
    // unit type. If not, it will return false.
    char msg[255];
    sprintf(msg, "canBuildUnit: Wanting to build iUnitType = [%d(=%s)] allowed?...", iUnitType, unitInfo[iUnitType].name);
    log(msg);

    // CHECK 1: Do we have the money?
    if (checkIfAffordable) {
        if (!hasEnoughCreditsForUnit(iUnitType)) {
            char msg[255];
            sprintf(msg, "canBuildUnit: FALSE, because cost %d higher than credits %d", unitInfo[iUnitType].cost, getCredits());
            log(msg);
            return eCantBuildReason::NOT_ENOUGH_MONEY; // NOPE
        }
    }

    // CHECK 2: Are we building this unit type?
    if (isBuildingSomethingInSameListSubListAsUnitType(iUnitType)) {
        char msg[255];
        sprintf(msg, "canBuildUnit: FALSE, because already building unitType");
        log(msg);
        return eCantBuildReason::ALREADY_BUILDING;
    }

    int iStrucType = structureUtils.getStructureTypeByUnitBuildId(iUnitType);

    // Do the reality-check, do we have the building needed?
    if (!hasAtleastOneStructure(iStrucType)) {
        char msg[255];
        sprintf(msg, "canBuildUnit: FALSE, because we do not own the required structure type [%s] for this unit: [%s]",
                structures[iStrucType].name, unitInfo[iUnitType].name);
        log(msg);
        return eCantBuildReason::REQUIRES_STRUCTURE;
    }

    if (iUnitType == DEVASTATOR || iUnitType == SONICTANK || iUnitType == DEVIATOR) {
        if (!hasAtleastOneStructure(IX)) {
            char msg[255];
            sprintf(msg, "canBuildUnit: FALSE, because we do not own the required ADDITIONAL structure type [%s] for this unit: [%s]",
                    structures[IX].name, unitInfo[iUnitType].name);
            log(msg);
            return eCantBuildReason::REQUIRES_ADDITIONAL_STRUCTURE;
        }
    }

    if (!canBuildUnitType(iUnitType)) {
        // not available to build (not in list)
        // assume it requires an upgrade?
        char msg[255];
        sprintf(msg, "canBuildUnit: REQUIRES_UPGRADE, because we can't find it in the expected list [%d] for this unit: [%s]",
                unitInfo[iUnitType].listId, unitInfo[iUnitType].name);
        log(msg);
        return eCantBuildReason::REQUIRES_UPGRADE;
    }

    log("canBuildUnit: ALLOWED");

    return eCantBuildReason::NONE;
}

eCantBuildReason cPlayer::canBuildSpecial(int iType) {
    // Once known, a check will be made to see if the AI has a structure to produce that
    // unit type. If not, it will return false.
    char msg[255];
    s_Special &special = specials[iType];
    sprintf(msg, "canBuildSpecial: Wanting to build iType = [%d(=%s)] allowed?...", iType, special.description);
    log(msg);

    // Do we have the building needed?
    int iStrucType = PALACE; // TODO: get from "special" data structure?
    if (!hasAtleastOneStructure(iStrucType)) {
        char msg[255];
        sprintf(msg, "canBuildUnit: FALSE, because we do not own the required structure type [%s] for [%s]",
                structures[iStrucType].name, special.description);
        log(msg);
        return eCantBuildReason::REQUIRES_STRUCTURE;
    }

    // Are we building this thing already?
    if (isBuildingAnythingForListAndSublist(special.listId, special.subListId)) {
        char msg[255];
        sprintf(msg, "canBuildSpecial: FALSE, because already building it");
        log(msg);
        return eCantBuildReason::ALREADY_BUILDING;
    }

    if (!canBuildSpecialType(iType)) {
        // not available to build (not in list)
        // assume it requires an upgrade?
        char msg[255];
        sprintf(msg, "canBuildUnit: REQUIRES_UPGRADE, because we can't find it in the expected list [%d] for this special: [%s]",
                special.listId, special.description);
        log(msg);
        return eCantBuildReason::REQUIRES_UPGRADE;
    }

    log("canBuildSpecial: ALLOWED");

    return eCantBuildReason::NONE;
}

eCantBuildReason cPlayer::canBuildUnit(int iUnitType) {
    return canBuildUnit(iUnitType, true);
}

bool cPlayer::canBuildUnitBool(int iUnitType) {
    return canBuildUnit(iUnitType, false) == eCantBuildReason::NONE;
}

int cPlayer::findRandomUnitTarget(int playerIndexToAttack) {
    // Randomly assemble list, and then pick one
    int iTargets[100];
    memset(iTargets, -1, sizeof(iTargets));

    int maxTargets = 0;

    for (int i = 0; i < MAX_UNITS; i++) {
        cUnit &cUnit = unit[i];
        if (!cUnit.isValid()) continue;
        if (cUnit.iPlayer != playerIndexToAttack) continue;
        // unit belongs to player of the player we wish to attack

        bool isVisibleForPlayer = map.isVisible(cUnit.getCell(), this);

        if (DEBUGGING) {
            char msg[255];
            sprintf(msg, "Visible = %d", isVisibleForPlayer);
            log(msg);
        }

        // HACK HACK: the AI player does not need to discover an enemy player yet
        if (isVisibleForPlayer || game.bSkirmish) {
            iTargets[maxTargets] = i;
            maxTargets++;

            if (maxTargets > 99)
                break;
        }
    }

    if (DEBUGGING) {
        char msg[255];
        sprintf(msg, "Targets %d", maxTargets);
        log(msg);
    }

    return iTargets[rnd(maxTargets)];
}

int cPlayer::findRandomStructureTarget(int iAttackPlayer) {
    // Randomly assemble list, and then pick one
    int iTargets[100];
    memset(iTargets, -1, sizeof(iTargets));

    int iT = 0;

    for (int i = 0; i < MAX_STRUCTURES; i++)
        if (structure[i])
            if (structure[i]->getOwner() == iAttackPlayer)
                if (map.isVisible(structure[i]->getCell(), this) ||
                    game.bSkirmish) {
                    iTargets[iT] = i;

                    iT++;

                    if (iT > 99)
                        break;
                }


    if (DEBUGGING) {
        char msg[255];
        sprintf(msg, "STR] Targets %d", iT);
        log(msg);
    }


    return (iTargets[rnd(iT)]);
}

eCantBuildReason cPlayer::canBuildStructure(int iStructureType) {
    char msg[255];
    sprintf(msg, "canBuildStructure: Wanting to build iStructureType = [%d]", iStructureType);
    log(msg);

    assert(iStructureType > -1 && "Structure type must be > -1");
    assert(iStructureType < MAX_STRUCTURETYPES && "Structure type must be < MAX_STRUCTURETYPES");

    // Once known, a check will be made to see if the AI has a structure to produce that
    // unit type. If not, it will return false.
    const s_Structures &structureType = structures[iStructureType];
    sprintf(msg, "canBuildStructure: Wanting to build iStructureType = [%d(=%s)], allowed?...",
            iStructureType, structureType.name);
    log(msg);

    // CHECK 1: Do we have the money?
    if (!hasEnoughCreditsForStructure(iStructureType)) {
        char msg[255];
        sprintf(msg, "canBuildStructure: FALSE, reason NOT_ENOUGH_MONEY: because cost %d > credits %d", structureType.cost,
                getCredits());
        log(msg);
        return eCantBuildReason::NOT_ENOUGH_MONEY; // NOPE
    }

    // Do the reality-check, do we have the building needed?
    if (!hasAtleastOneStructure(CONSTYARD)) {
        char msg[255];
        sprintf(msg,
                "canBuildStructure: FALSE, reason REQUIRES_STRUCTURE: we do not own the required structure type [%s] for this structure: [%s]",
                structures[CONSTYARD].name, structureType.name);
        log(msg);
        return eCantBuildReason::REQUIRES_STRUCTURE;
    }

    // CHECK 2: Are we building another structure already?
    if (isBuildingStructure()) {
        char msg[255];
        sprintf(msg, "canBuildStructure: FALSE, reason ALREADY_BUILDING");
        log(msg);
        return eCantBuildReason::ALREADY_BUILDING;
    }

    if (!canBuildStructureType(iStructureType)) {
        // not available to build (not in list)
        // assume it requires an upgrade?
        char msg[255];
        sprintf(msg, "canBuildStructure: FALSE, reason REQUIRES_UPGRADE: because we can't find structure in list");
        log(msg);
        return eCantBuildReason::REQUIRES_UPGRADE;
    }

    log("canBuildStructure: ALLOWED");

    return eCantBuildReason::NONE;
}

cAbstractStructure *cPlayer::placeStructure(int destinationCell, int iStructureTypeId, int healthPercentage) {
    // create structure
    cStructureFactory *pStructureFactory = cStructureFactory::getInstance();
    return pStructureFactory->createStructure(destinationCell, iStructureTypeId, getId(), healthPercentage);
}

cAbstractStructure *cPlayer::placeStructure(int destinationCell, cBuildingListItem *itemToPlace) {
    int iStructureTypeId = itemToPlace->getBuildId();
    // create structure
    cStructureFactory *pStructureFactory = cStructureFactory::getInstance();

    bool canPlace = canPlaceStructureAt(destinationCell, iStructureTypeId).success;
    if (!canPlace) {
        return nullptr;
    }

    if (autoSlabStructures) {
        pStructureFactory->slabStructure(destinationCell, iStructureTypeId, getId());
    }

    int slabbed = pStructureFactory->getSlabStatus(destinationCell, iStructureTypeId);
    int height = structures[iStructureTypeId].bmp_height / TILESIZE_HEIGHT_PIXELS;
    int width = structures[iStructureTypeId].bmp_width / TILESIZE_WIDTH_PIXELS;
    int surface = width * height;

    int healthPercentage = 50 + health_bar(50, slabbed, surface); // the minimum is 50% (with no slabs)

    cAbstractStructure *pStructure = pStructureFactory->createStructure(destinationCell,
                                                                        iStructureTypeId,
                                                                        getId(),
                                                                        healthPercentage);

    buildingListUpdater->onBuildItemCompleted(itemToPlace);
    itemToPlace->decreaseTimesToBuild();
    itemToPlace->setPlaceIt(false);
    itemToPlace->setIsBuilding(false);
    itemToPlace->resetProgress();
    if (itemToPlace->getTimesToBuild() < 1) {
        itemBuilder->removeItemFromList(itemToPlace);
    }

    return pStructure;
}

void cPlayer::onNotify(const s_GameEvent &event) {
    // notify building list updater if it was a structure of mine. So it gets removed from the building list.
    if (event.player == this) {
        if (event.entityType == eBuildType::STRUCTURE) {
            if (event.eventType == eGameEventType::GAME_EVENT_DESTROYED) {
                buildingListUpdater->onStructureDestroyed(event.entitySpecificType);
            } else if (event.eventType == eGameEventType::GAME_EVENT_CREATED) {
                buildingListUpdater->onStructureCreated(event.entitySpecificType);
            }
        }
    }

    // pass event to brain
    if (brain_) {
        brain_->onNotify(event);
    }
}

void cPlayer::setAutoSlabStructures(bool value) {
    autoSlabStructures = value;
}

int cPlayer::getScoutingUnitType() {
    // TODO: make this configurable within house props
    switch (house) {
        case HARKONNEN:
            return QUAD;
        case ATREIDES:
            return TRIKE;
        case ORDOS:
            return RAIDER;
        case SARDAUKAR:
            return QUAD;
        case FREMEN:
            return TRIKE;
    }

    return TRIKE;
}

int cPlayer::getSpecialUnitType() {
    // TODO: make this configurable within house props
    switch (house) {
        case HARKONNEN:
            return DEVASTATOR;
        case ATREIDES:
            return SONICTANK;
        case ORDOS:
            return DEVIATOR;
        case SARDAUKAR:
            return DEVASTATOR;
        case FREMEN:
            return TRIKE;
    }

    return TRIKE;
}

bool cPlayer::hasEnoughPowerFor(int structureType) const {
    assert(structureType > -1 && "hasEnoughPowerFor called with structureType < 0!");
    assert(structureType < MAX_STRUCTURETYPES && "hasEnoughPowerFor called with structureType >= MAX_STRUCTURETYPES!");
    int powerLeft = powerProduce_ - powerUsage_;
    return structures[structureType].power_drain <= powerLeft;
}

void cPlayer::logStructures() {
    log("cPlayer::logStructures() START");
    for (int i = 0; i < MAX_STRUCTURETYPES; i++) {
        char msg[255];
        sprintf(msg, "[%s] amount [%d]", structures[i].name, iStructures[i]);
        log(msg);
    }
    log("cPlayer::logStructures() END");
}

void cPlayer::cancelBuildingListItem(cBuildingListItem *item) {
    this->sidebar->cancelBuildingListItem(item);
}

void cPlayer::cancelStructureBuildingListItemBeingBuilt() {
    cBuildingListItem *pItem = getStructureBuildingListItemBeingBuilt();
    if (pItem) {
        cancelBuildingListItem(pItem);
    }
}

/**
<p>
	This function will check if at iCell (the upper left corner of a structure) a structure
	can be placed of type "iStructureType". This is calling  canPlaceStructureAt without any
    unitID to ignore. Meaning, any unit, structure, or invalid terrain type will make this function return false.
 </p>
 <p>
 <b>Returns:</b><br>
 <ul>
 <li>result object</li>
 <ul>
 </p>

 * @param iCell
 * @param iStructureType
 * @param iUnitIDToIgnore
 * @return
 */
s_PlaceResult cPlayer::canPlaceStructureAt(int iCell, int iStructureType) {
    return canPlaceStructureAt(iCell, iStructureType, -1);
}


/**
<p>
	This function will check if at iCell (the upper left corner of a structure) a structure
	can be placed of type "iStructureType". If iUnitIDTOIgnore is > -1, then if any unit is
	supposedly 'blocking' this structure from placing, it will be ignored.
 </p>
<p>
	Ie, you will use the iUnitIDToIgnore value when you want to create a Const Yard on the
	location of an MCV.
</p>
 <p>
	If you know the structure can be placed, you can use getSlabStatus to get the amount of 'slabs' are covering
    the structure dimensions in order to calculate the structure health upon placement.
</p>
 <p>
 <b>Returns:</b><br>
 <ul>
 <li>s_PlaceResult = if success there is nothing in the way to place structure</li>
 <ul>
 </p>

 * @param iCell
 * @param iStructureType
 * @param iUnitIDToIgnore
 * @return
 */
s_PlaceResult cPlayer::canPlaceStructureAt(int iCell, int iStructureType, int iUnitIDToIgnore) {
    s_PlaceResult result;

    if (!map.isValidCell(iCell)) {
        result.outOfBounds = true;
        return result;
    }

    // checks if this structure can be placed on this cell
    int w = structures[iStructureType].bmp_width/TILESIZE_WIDTH_PIXELS;
    int h = structures[iStructureType].bmp_height/TILESIZE_HEIGHT_PIXELS;

    int x = map.getCellX(iCell);
    int y = map.getCellY(iCell);

    bool foundUnitFromOtherPlayerThanMe = false;

    for (int cx = 0; cx < w; cx++) {
        for (int cy = 0; cy < h; cy++) {
            int cll = map.getCellWithMapBorders(cx + x, cy + y);

            if (!result.badTerrain && !map.isValidTerrainForStructureAtCell(cll)) {
                result.badTerrain = true;
            }

            // another structure found on this location, "blocked"
            int structureId = map.getCellIdStructuresLayer(cll);
            if (structureId > -1) {
                result.structureIds.insert(structureId);
            }

            int idOfUnitAtCell = map.getCellIdUnitLayer(cll);
            if (idOfUnitAtCell > -1) {
                if (unit[idOfUnitAtCell].isValid() && unit[idOfUnitAtCell].getPlayer() != this) {
                    foundUnitFromOtherPlayerThanMe = true;
                }
                if (iUnitIDToIgnore > -1) {
                    if (idOfUnitAtCell != iUnitIDToIgnore) {
                        result.unitIds.insert(idOfUnitAtCell);
                    }
                } else {
                    result.unitIds.insert(idOfUnitAtCell);
                }
            }
        }
    }

    result.success = (result.badTerrain == false && result.unitIds.empty() && result.structureIds.empty());
    result.onlyMyUnitsBlock = (result.badTerrain == false && !foundUnitFromOtherPlayerThanMe && result.structureIds.empty());

    return result;
}

void cPlayer::log(const char *txt) const {
    if (DEBUGGING) {
        char msg[1024];
        sprintf(msg, "PLAYER [%d(=%s)] : %s", getId(), getHouseName().c_str(), txt);
        logbook(msg);
    }
}
