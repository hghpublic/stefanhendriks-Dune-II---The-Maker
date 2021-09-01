#include "include/d2tmh.h"
#include "cPlayerBrainSandworm.h"

namespace brains {
    cPlayerBrainSandworm::cPlayerBrainSandworm(cPlayer *player) : cPlayerBrain(player) {
        TIMER_think = 0;
    }

    cPlayerBrainSandworm::~cPlayerBrainSandworm() {

    }

    void cPlayerBrainSandworm::think() {
//        char msg[255];
//        sprintf(msg, "cPlayerBrainSandworm::think()");
//        log(msg);
        TIMER_think++;
        if (TIMER_think < 10) {
            return;
        }

        TIMER_think = 0;
        // TODO: make this more smart, ie, keep a collection of pointers in player, so we don't
        // need to loop over ALL units all the time.

//        this->player->getAllMyUnits();
        // loop through all its worms and move them around
        for (int i = 0; i < MAX_UNITS; i++) {
            cUnit &pUnit = unit[i];
            if (!pUnit.isValid()) continue;
            if (!pUnit.isSandworm()) continue;
            if (pUnit.getPlayer() != player) continue;

            // when on guard
            if (pUnit.isIdle()) {
                // find new spot to go to
                for (int iTries = 0; iTries < 10; iTries++) {
                    int iMoveTo = map.getRandomCell();

                    if (map.isCellPassableForWorm(iMoveTo)) {
                        pUnit.move_to(iMoveTo);
                        continue;
                    }
                }
            }
        }

    }

    void cPlayerBrainSandworm::onNotify(const s_GameEvent &event) {

    }

    void cPlayerBrainSandworm::addBuildOrder(S_buildOrder order) {

    }

    void cPlayerBrainSandworm::thinkFast() {

    }
}
