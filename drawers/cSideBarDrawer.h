/*
 * cSideBarDrawer.h
 *
 *  Created on: Aug 2, 2009
 *      Author: Stefan
 */

#ifndef CSIDEBARDRAWER_H_
#define CSIDEBARDRAWER_H_

class cSideBarDrawer {
	public:
		cSideBarDrawer();
		virtual ~cSideBarDrawer();

		void drawSideBar(const cPlayer & player);

		cBuildingListDrawer * getBuildingListDrawer() { return buildingListDrawer; }

	protected:
		void drawHouseGui(const cPlayer & thePlayer);
		void drawBuildingLists(const cPlayer & thePlayer);
		void drawCapacities(const cPlayer & thePlayer);

		void drawCandybar();

		void drawMinimap(const cPlayer & player);
		void drawOptionsBar(const cPlayer & thePlayer);
	private:
		cBuildingListDrawer * buildingListDrawer;

		// the 'candybar' is the bar with the ball at the top. Colored with the house color.
		BITMAP *candybar;
		BITMAP *optionsBar;
		cTextDrawer *textDrawer;

		int sidebarColor;

        void createCandyBar();

        void drawPowerUsage(const cPlayer &player) const;

    void drawCreditsUsage(const cPlayer &player);
};

#endif /* CSIDEBARDRAWER_H_ */
