/*
 * =====================================================================================
 *
 *       Filename: myhero.cpp
 *        Created: 08/31/2015 08:52:57 PM
 *  Last Modified: 12/12/2017 17:30:38
 *
 *    Description: 
 *
 *        Version: 1.0
 *       Revision: none
 *       Compiler: gcc
 *
 *         Author: ANHONG
 *          Email: anhonghe@gmail.com
 *   Organization: USTC
 *
 * =====================================================================================
 */

#include "log.hpp"
#include "game.hpp"
#include "myhero.hpp"
#include "message.hpp"
#include "mathfunc.hpp"
#include "clientenv.hpp"
#include "processrun.hpp"
#include "clientpathfinder.hpp"

MyHero::MyHero(uint32_t nUID, uint32_t nDBID, bool bMale, uint32_t nDressID, ProcessRun *pRun, const ActionNode &rstAction)
	: Hero(nUID, nDBID, bMale, nDressID, pRun, rstAction)
    , m_Gold(0)
    , m_InvPack()
    , m_ActionQueue()
{}

bool MyHero::Update(double fUpdateTime)
{
    // 1. independent from time control
    //    attached magic could take different speed
    UpdateAttachMagic(fUpdateTime);

    // 2. update this monster
    //    need fps control for current motion
    double fTimeNow = SDL_GetTicks() * 1.0;
    if(fTimeNow > CurrMotionDelay() + m_LastUpdateTime){

        // 1. record update time
        //    needed for next update
        m_LastUpdateTime = fTimeNow;

        switch(m_CurrMotion.Motion){
            case MOTION_STAND:
                {
                    if(m_MotionQueue.empty()){
                        if(m_ActionQueue.empty()){
                            return AdvanceMotionFrame(1);
                        }else{
                            return ParseActionQueue();
                        }
                    }else{
                        // move to next motion will reset frame as 0
                        // if current there is no more motion pending
                        // it will add a MOTION_STAND
                        //
                        // we don't want to reset the frame here
                        return MoveNextMotion();
                    }
                }
            case MOTION_HITTED:
                {
                    if(m_MotionQueue.empty()){
                        if(m_ActionQueue.empty()){
                            return UpdateMotion(false);
                        }else{
                            return ParseActionQueue();
                        }
                    }else{
                        // move to next motion will reset frame as 0
                        // if current there is no more motion pending
                        // it will add a MOTION_STAND
                        //
                        // we don't want to reset the frame here
                        return MoveNextMotion();
                    }
                }
            default:
                {
                    return UpdateMotion(false);
                }
        }
    }
    return true;
}

bool MyHero::MoveNextMotion()
{
    if(m_MotionQueue.empty()){
        if(m_ActionQueue.empty()){
            m_CurrMotion = {
                MOTION_STAND,
                0,
                m_CurrMotion.Direction,
                m_CurrMotion.EndX,
                m_CurrMotion.EndY
            };
            return true;
        }else{
            // there is pending action in the queue
            // try to present it and return false if failed
            return ParseActionQueue();
        }
    }

    if(MotionQueueValid()){
        m_CurrMotion = m_MotionQueue.front();
        m_MotionQueue.pop_front();
        return true;
    }

    // oops we get invalid motion queue
    extern Log *g_Log;
    g_Log->AddLog(LOGTYPE_INFO, "Invalid motion queue:");

    m_CurrMotion.Print();
    for(auto &rstMotion: m_MotionQueue){
        rstMotion.Print();
    }

    extern Log *g_Log;
    g_Log->AddLog(LOGTYPE_FATAL, "Current motion is not valid");
    return false;
}

bool MyHero::DecompMove(bool bCheckGround, bool bCheckCreature, bool bCheckMove, int nX0, int nY0, int nX1, int nY1, int *pXm, int *pYm)
{
    auto stvPathNode = ParseMovePath(nX0, nY0, nX1, nY1, bCheckGround, bCheckCreature);
    switch(stvPathNode.size()){
        case 0:
        case 1:
            {
                return false;
            }
        default:
            {
                if(bCheckMove){

                    // request to check if the first step is possible
                    // to avoid creature to move into invalid or occupied grids

                    auto nXt = stvPathNode[1].X;
                    auto nYt = stvPathNode[1].Y;

                    int nDX = (nXt > nX0) - (nXt < nX0);
                    int nDY = (nYt > nY0) - (nYt < nY0);

                    int nIndexMax = 0;
                    switch(LDistance2(nX0, nY0, nXt, nYt)){
                        case 1:
                        case 2:
                            {
                                nIndexMax = 1;
                                break;
                            }
                        case 4:
                        case 8:
                            {
                                nIndexMax = 2;
                                break;
                            }
                        case  9:
                        case 18:
                            {
                                nIndexMax = 3;
                                break;
                            }
                        default:
                            {
                                return false;
                            }
                    }

                    int nReachIndexMax = 0;
                    for(int nIndex = 1; nIndex <= nIndexMax; ++nIndex){
                        if(m_ProcessRun->CanMove(true, nX0 + nDX * nIndex, nY0 + nDY * nIndex)){
                            nReachIndexMax = nIndex;
                        }else{ break; }
                    }

                    switch(nReachIndexMax){
                        case 0:
                            {
                                // we find an impossible step
                                // need to reject and report failure
                                return false;
                            }
                        default:
                            {
                                // we allow step size as 1, 2, 3
                                // every creature has two possible step size 1, MaxStep

                                // so if it failed to reach grid with MaxStep size
                                // it should use sizeStep as 1 only

                                // like if human on horse failed for a MOTION_HORSERUN
                                // then it should only use MOTION_HORSEWALK, rather than MOTION_RUN

                                if(pXm){ *pXm = (nReachIndexMax == nIndexMax) ? nXt : (nX0 + nDX); }
                                if(pYm){ *pYm = (nReachIndexMax == nIndexMax) ? nYt : (nY0 + nDY); }

                                return true;
                            }
                    }

                }else{

                    // won't check if srcLoc -> midLoc is possible
                    // 1. could contain invalid grids if not set bCheckGround
                    // 2. could contain occuped grids if not set bCheckCreature

                    if(pXm){ *pXm = stvPathNode[1].X; }
                    if(pYm){ *pYm = stvPathNode[1].Y; }

                    return true;
                }
            }
    }
}

bool MyHero::DecompActionPickUp()
{
    if(true
            && !m_ActionQueue.empty()
            &&  m_ActionQueue.front().Action == ACTION_PICKUP){

        auto stCurrPickUp = m_ActionQueue.front();
        m_ActionQueue.pop_front();

        int nX0 = m_CurrMotion.EndX;
        int nY0 = m_CurrMotion.EndY;
        int nX1 = stCurrPickUp.X;
        int nY1 = stCurrPickUp.Y;

        if(!m_ProcessRun->CanMove(false, nX0, nY0)){
            extern Log *g_Log;
            g_Log->AddLog(LOGTYPE_WARNING, "Motion start from invalid grid (%d, %d)", nX0, nY0);

            m_ActionQueue.clear();
            return false;
        }

        if(!m_ProcessRun->CanMove(false, nX1, nY1)){
            extern Log *g_Log;
            g_Log->AddLog(LOGTYPE_WARNING, "Pick up at an invalid grid (%d, %d)", nX1, nY1);

            m_ActionQueue.clear();
            return false;
        }

        switch(LDistance2(nX0, nY0, nX1, nY1)){
            case 0:
                {
                    m_ActionQueue.emplace_front(stCurrPickUp);
                    return true;
                }
            default:
                {
                    int nXm = -1;
                    int nYm = -1;

                    if(DecompMove(true, true, true, nX0, nY0, nX1, nY1, &nXm, &nYm)){
                        m_ActionQueue.emplace_front(stCurrPickUp);
                        m_ActionQueue.emplace_front(ActionMove(nX0, nY0, nXm, nYm, SYS_DEFSPEED, OnHorse() ? 1 : 0));
                        return true;
                    }else{
                        if(DecompMove(true, false, false, nX0, nY0, nX1, nY1, nullptr, nullptr)){
                            // reachable but blocked
                            // path occupied, we restore it and wait
                            m_ActionQueue.emplace_front(stCurrPickUp);
                        }

                        // report failure
                        // head is not simple action
                        return false;
                    }
                }
        }
    }
    return false;
}

bool MyHero::DecompActionMove()
{
    if(true
            && !m_ActionQueue.empty()
            &&  m_ActionQueue.front().Action == ACTION_MOVE){

        auto stCurrMove = m_ActionQueue.front();
        m_ActionQueue.pop_front();

        int nX0 = stCurrMove.X;
        int nY0 = stCurrMove.Y;
        int nX1 = stCurrMove.AimX;
        int nY1 = stCurrMove.AimY;

        if(!m_ProcessRun->CanMove(false, nX0, nY0)){
            extern Log *g_Log;
            g_Log->AddLog(LOGTYPE_WARNING, "Motion start from invalid grid (%d, %d)", nX0, nY0);

            m_ActionQueue.clear();
            return false;
        }

        switch(LDistance2(nX0, nY0, nX1, nY1)){
            case 0:
                {
                    extern Log *g_Log;
                    g_Log->AddLog(LOGTYPE_WARNING, "Motion invalid (%d, %d) -> (%d, %d)", nX0, nY0, nX1, nY1);

                    // I have to clear all pending actions
                    // because if I return true the next step treats the front action as basic one

                    m_ActionQueue.clear();
                    return false;
                }
            default:
                {
                    auto fnAddHop = [this, stCurrMove](int nXm, int nYm) -> bool
                    {
                        switch(LDistance2(stCurrMove.AimX, stCurrMove.AimY, nXm, nYm)){
                            case 0:
                                {
                                    m_ActionQueue.emplace_front(stCurrMove);
                                    return true;
                                }
                            default:
                                {
                                    m_ActionQueue.emplace_front(ActionMove(nXm, nYm, stCurrMove.AimX, stCurrMove.AimY, stCurrMove.Speed, 0));
                                    m_ActionQueue.emplace_front(ActionMove(stCurrMove.X, stCurrMove.Y, nXm, nYm, stCurrMove.Speed, 0));
                                    return true;
                                }
                        }
                    };

                    int nXm = -1;
                    int nYm = -1;

                    bool bCheckGround = m_ProcessRun->CanMove(false, nX1, nY1);
                    if(DecompMove(bCheckGround, true, true, nX0, nY0, nX1, nY1, &nXm, &nYm)){
                        return fnAddHop(nXm, nYm);
                    }else{
                        if(bCheckGround){
                            // means there is no such way to there
                            // move as much as possible
                            if(DecompMove(false, true, true, nX0, nY0, nX1, nY1, &nXm, &nYm)){
                                return fnAddHop(nXm, nYm);
                            }else{
                                // won't check the ground but failed
                                // only one possibility: the first step is not legal
                                return false;
                            }
                        }else{
                            return false;
                        }
                    }
                }
                break;
        }
    }
    return false;
}

bool MyHero::DecompActionAttack()
{
    if(true
            && !m_ActionQueue.empty()
            &&  m_ActionQueue.front().Action == ACTION_ATTACK){

        auto stCurrAction = m_ActionQueue.front();
        m_ActionQueue.pop_front();

        // when parsing ActionAttack
        // we don't use ActionAttack::X/Y

        // this X/Y is used to send to the server
        // for location verification only

        auto nX0 = m_CurrMotion.EndX;
        auto nY0 = m_CurrMotion.EndY;

        // use if need to keep the attack node
        // we use m_CurrMotion.EndX/Y instead of rstAction.X/Y

        ActionAttack stAttack
        {
            nX0,
            nY0,
            (int)(stCurrAction.ActionParam),
            stCurrAction.Speed,
            stCurrAction.AimUID,
        };

        if(auto pCreature = m_ProcessRun->RetrieveUID(stCurrAction.AimUID)){
            auto nX1 = pCreature->X();
            auto nY1 = pCreature->Y();

            switch(LDistance2(nX0, nY0, nX1, nY1)){
                case 0:
                    {
                        extern Log *g_Log;
                        g_Log->AddLog(LOGTYPE_WARNING, "Invalid attack location (%d, %d) -> (%d, %d)", nX0, nY0, nX1, nY1);

                        m_ActionQueue.clear();
                        return false;
                    }
                case 1:
                case 2:
                    {
                        m_ActionQueue.emplace_front(stAttack);
                        return true;
                    }
                default:
                    {
                        // not close enough for the PLAIN_PHY_ATTACK
                        // need to schedule a path to move closer and then attack

                        int nXt = -1;
                        int nYt = -1;

                        if(DecompMove(true, true, true, nX0, nY0, nX1, nY1, &nXt, &nYt)){

                            // decompse the move
                            // but need to check if it's one step distance

                            switch(LDistance2(nXt, nYt, nX1, nY1)){
                                case 0:
                                    {
                                        // one hop we can reach the attack location
                                        // but we know step size between (nX0, nY0) and (nX1, nY1) > 1

                                        int nXm  = -1;
                                        int nYm  = -1;
                                        int nDir = PathFind::GetDirection(nX0, nY0, nX1, nY1);
                                        PathFind::GetFrontLocation(&nXm, &nYm, nX0, nY0, nDir, 1);

                                        nXt = nXm;
                                        nYt = nYm;

                                        break;
                                    }
                                default:
                                    {
                                        break;
                                    }
                            }

                            m_ActionQueue.emplace_front(ActionAttack
                            {
                                nXt,
                                nYt,
                                (int)(stCurrAction.ActionParam),
                                stCurrAction.Speed,
                                stCurrAction.AimUID,
                            });

                            m_ActionQueue.emplace_front(ActionMove
                            {
                                nX0,
                                nY0,
                                nXt,
                                nYt,
                                SYS_DEFSPEED,
                                (OnHorse() ? 1 : 0),
                            });

                            return true;
                        }else{

                            // decompse failed, why it failed?
                            // if can't reach we need to reject current action
                            // if caused by occupied grids of creatures, we need to keep it

                            if(DecompMove(true, false, false, nX0, nY0, nX1, nY1, nullptr, nullptr)){
                                // keep it
                                // can reach but not now
                                m_ActionQueue.emplace_front(stAttack);
                            }else{
                                m_ActionQueue.clear();
                            }

                            // decompse failed
                            // the head of the action queue is not simple
                            return false;
                        }
                    }
            }
        }

        // we can't get the UID
        // remove the attack node and return false
        return false;
    }
    return false;
}

bool MyHero::DecompActionSpell()
{
    if(true
            && !m_ActionQueue.empty()
            &&  m_ActionQueue.front().Action == ACTION_SPELL){

        auto stCurrAction = m_ActionQueue.front();
        m_ActionQueue.pop_front();

        // like dual axe
        // some magic it has a attack distance

        m_ActionQueue.emplace_front(stCurrAction);
        return true;
    }
    return false;
}

bool MyHero::ParseActionQueue()
{
    if(m_ActionQueue.empty()){
        return true;
    }

    // trace message
    // trace move action before parsing
    {
        extern ClientEnv *g_ClientEnv;
        if(g_ClientEnv->TraceMove){
            if((!m_ActionQueue.empty()) && (m_ActionQueue.front().Action == ACTION_MOVE)){
                auto nMotionX0 = m_CurrMotion.X;
                auto nMotionY0 = m_CurrMotion.Y;
                auto nMotionX1 = m_CurrMotion.EndX;
                auto nMotionY1 = m_CurrMotion.EndY;

                auto nActionX0 = m_ActionQueue.front().X;
                auto nActionY0 = m_ActionQueue.front().Y;
                auto nActionX1 = m_ActionQueue.front().AimX;
                auto nActionY1 = m_ActionQueue.front().AimY;

                extern Log *g_Log;
                g_Log->AddLog(LOGTYPE_INFO, "BF: CurrMotion: (%d, %d) -> (%d, %d)", nMotionX0, nMotionY0, nMotionX1, nMotionY1);
                g_Log->AddLog(LOGTYPE_INFO, "BF: CurrAction: (%d, %d) -> (%d, %d)", nActionX0, nActionY0, nActionX1, nActionY1);
            }
        }
    }

    // parsing action
    // means pending motion queue must be empty
    condcheck(m_MotionQueue.empty());

    // all actions in action queue is local and not verfified
    // present the action list immediately and sent it to server for verification
    // the server will keep silent if permitted, or send pull-back message if rejected

    // 1. decompose action into simple action if needed
    // 2. send the simple action to server for verification
    // 3. present the action simultaneously

    switch(m_ActionQueue.front().Action){
        case ACTION_PICKUP:
            {
                if(!DecompActionPickUp()){
                    return false;
                }
                break;
            }
        case ACTION_MOVE:
            {
                if(!DecompActionMove()){
                    return false;
                }
                break;
            }
        case ACTION_ATTACK:
            {
                if(!DecompActionAttack()){
                    return false;
                }
                break;
            }
        case ACTION_SPELL:
            {
                if(!DecompActionSpell()){
                    return false;
                }
                break;
            }
        default:
            {
                break;
            }
    }

    // pick the first simple action and handle it
    {
        auto stCurrAction = m_ActionQueue.front();
        m_ActionQueue.pop_front();

        ReportAction(ActionNode
        {
            stCurrAction.Action,
            stCurrAction.Speed,
            stCurrAction.Direction,
            stCurrAction.X,
            stCurrAction.Y,
            stCurrAction.AimX,
            stCurrAction.AimY,
            stCurrAction.AimUID,
            stCurrAction.ActionParam,
        });

        // present current *local* action without verification
        // later if server refused current action we'll do correction by pullback

        if(ParseAction(stCurrAction)){
            // trace message
            // trace move action after parsing
            {
                extern ClientEnv *g_ClientEnv;
                if(g_ClientEnv->TraceMove){
                    for(auto &rstMotion: m_MotionQueue){
                        auto nMotionX0 = rstMotion.X;
                        auto nMotionY0 = rstMotion.Y;
                        auto nMotionX1 = rstMotion.EndX;
                        auto nMotionY1 = rstMotion.EndY;

                        extern Log *g_Log;
                        g_Log->AddLog(LOGTYPE_INFO, "AF: CurrMotion: (%d, %d) -> (%d, %d)", nMotionX0, nMotionY0, nMotionX1, nMotionY1);
                    }

                    if(m_ActionQueue.empty()){
                        extern Log *g_Log;
                        g_Log->AddLog(LOGTYPE_INFO, "AF: CurrAction: NONE");
                    }else{
                        for(auto &rstAction: m_ActionQueue){
                            auto nActionX0 = rstAction.X;
                            auto nActionY0 = rstAction.Y;
                            auto nActionX1 = rstAction.AimX;
                            auto nActionY1 = rstAction.AimY;

                            extern Log *g_Log;
                            g_Log->AddLog(LOGTYPE_INFO, "AF: CurrAction: (%d, %d) -> (%d, %d)", nActionX0, nActionY0, nActionX1, nActionY1);
                        }
                    }
                }
            }

            // ParseAction() can make m_MotionQueue empty
            // Like for ACTION_PICKUP

            if(!m_MotionQueue.empty()){
                m_CurrMotion = m_MotionQueue.front();
                m_MotionQueue.pop_front();
            }
        }else{
            return false;
        }
    }
    return true;
}

bool MyHero::StayIdle()
{
    return true
        && m_MotionQueue.empty()
        && m_ActionQueue.empty();
}

void MyHero::PickUp()
{
    if(StayIdle()){
        for(auto &rstGroundItem: m_ProcessRun->GetGroundItemList()){
            if(true
                    && rstGroundItem.X == CurrMotion().X
                    && rstGroundItem.Y == CurrMotion().Y){
                ReportAction(ActionPickUp(rstGroundItem.X, rstGroundItem.Y, rstGroundItem.ID));
            }
        }
    }
}

bool MyHero::EmplaceAction(const ActionNode &rstAction)
{
    m_ActionQueue.clear();
    m_ActionQueue.push_back(rstAction);
    return true;
}

void MyHero::ReportAction(const ActionNode &rstAction)
{
    CMAction stCMA;
    std::memset(&stCMA, 0, sizeof(stCMA));

    stCMA.UID   = UID();
    stCMA.MapID = m_ProcessRun->MapID();

    stCMA.Action    = rstAction.Action;
    stCMA.Speed     = rstAction.Speed;
    stCMA.Direction = rstAction.Direction;

    stCMA.X    = rstAction.X;
    stCMA.Y    = rstAction.Y;
    stCMA.AimX = rstAction.AimX;
    stCMA.AimY = rstAction.AimY;

    stCMA.AimUID      = rstAction.AimUID;
    stCMA.ActionParam = rstAction.ActionParam;

    extern Game *g_Game;
    g_Game->Send(CM_ACTION, stCMA);
}
