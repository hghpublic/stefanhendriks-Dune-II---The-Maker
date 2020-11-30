/*
 * cBuildingListDrawer.h
 *
 *  Created on: Aug 1, 2009
 *      Author: Stefan
 */

#ifndef CBUILDINGLISTDRAWER_H_
#define CBUILDINGLISTDRAWER_H_

class cBuildingListDrawer {

public:
	cBuildingListDrawer();
	~cBuildingListDrawer();

	void drawList(cBuildingList *list, int listIDToDraw);
    void drawButtonHoverRectangle(cBuildingList *list);
	void drawButton(cBuildingList *list, bool pressed);

	cBuildingListItem *isOverItemCoordinates(cBuildingList *list, int x, int y);

    int getDrawX();
    int getDrawY();

protected:

private:
	void drawStructureSize(int structureId, int x, int y);
	void drawList(cBuildingList *list, int listIDToDraw, bool shouldDrawStructureSize);
	void drawListWithStructures(cBuildingList *list, int listIDToDraw);		// used for const yard
	void drawListWithUnitsOrAbilities(cBuildingList *list, int listIDToDraw); // used for units or palace / starport

	bool isOverItemCoordinates_Boolean(int x, int y, int drawX, int drawY);

	// the list to draw
	cBuildingList *list;
	cTextDrawer *textDrawer;
};

#endif /* CBUILDINGLISTDRAWER_H_ */
