/*
 * =====================================================================================
 *
 *       Filename: rotatecoord.cpp
 *        Created: 08/15/2015 04:01:57
 *  Last Modified: 06/13/2017 12:53:08
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

#include <algorithm>
#include "mathfunc.hpp"
#include "rotatecoord.hpp"

bool RotateCoord::Reset(int nCenterX, int nCenterY, int nStartX, int nStartY, int nW, int nH)
{
    if((nW > 0) && (nH > 0)){ 
        m_CenterX = nCenterX;
        m_CenterY = nCenterY;
        m_StartX  = nStartX;
        m_StartY  = nStartY;
        m_StopX   = nStartX + nW - 1;
        m_StopY   = nStartY + nH - 1;

        if(PointInRectangle(m_CenterX, m_CenterY, m_StartX, m_StartY, nW, nH)){
            m_Distance  = 0;
            m_Direction = 0;
            m_CurrentX  = m_CenterX;
            m_CurrentY  = m_CenterY;
        }else{
            // if center point is not inside the rectangle
            // we should firstly decide the rotation direction for initialization
            //
            //         (1)
            //      *-------*
            //      |*     *|
            //      | *   * |
            //      |  * *  |  (2)
            //  (4) |   * <-- CenterX, CenterY
            //      |  * *  |
            //      | *   * |
            //      |*     *|
            //      *-------*
            //         (3)

            // case-(1)
            // center pointer is at the top of the rectangle
            if(true
                    && ((m_CenterX - m_CenterY) >= (m_StartX - m_StartY))
                    && ((m_CenterX + m_CenterY) <  (m_StopX  + m_StartY))
                    && ((m_CenterY < m_StartY))){
                m_Distance  = m_StartY - m_CenterY;
                m_Direction = 0;
                m_CurrentX  = (std::max)(m_StartX, m_CenterX - m_Distance);
                m_CurrentY  = m_StartY;
            }

            // case-(2)
            // complicated part here
            // center pointer is at the right side of the rectangle
            // when set direction to 0 1 2 it's ok
            // but for 3, need to set the the smallest valid direction
            // because for direction = 3, there is jumping when forwarding
            if(true
                    && ((m_CenterX + m_CenterY) >= (m_StopX + m_StartY))
                    && ((m_CenterX - m_CenterY) >  (m_StopX - m_StopY ))
					&& ((m_CenterX > m_StopX))){

                m_Distance = m_CenterX - m_StopX;
                m_CurrentX = m_StopX;

                if(m_StopY >= m_CenterY + m_Distance){
                    // the vertical edge is long enough
                    // on it we can get one point as start point with direction as 0
                    m_Direction = 0;
                    m_CurrentY  = m_CenterY + m_Distance;
                }else{
                    // set start point with direction = 3
                    m_Direction = 3;
                    m_CurrentY  = (std::max)(m_StartY, m_CenterY - m_Distance);
                }
            }

            // case-(3)
            // center pointer is at the bottom of the rectangle
            if(true
                    && ((m_CenterX - m_CenterY) <= (m_StopX  - m_StopY))
                    && ((m_CenterX + m_CenterY) >  (m_StartX + m_StopY))
                    && ((m_CenterY > m_StopY))){

                m_Distance  = m_CenterY - m_StopY;
                m_Direction = 2;
                m_CurrentX  = (std::min)(m_StopX, m_CenterX + m_Distance);
                m_CurrentY  = m_StopY;
            }

            // case-(4)
            // center pointer is at the left side of the rectangle
            if(true
                    && ((m_CenterX + m_CenterY) <= (m_StartX + m_StopY ))
                    && ((m_CenterX - m_CenterY) <  (m_StartX - m_StartY))
                    && ((m_CenterX < m_StartX))){

                m_Distance  = m_StartX - m_CenterX;
                m_Direction = 1;
				m_CurrentX  = m_StartX;
                m_CurrentY  = (std::min)(m_StopY, m_CenterY + m_Distance);
            }
        }

        CheckOverlap();
        return true;
    }
    return false;
}

void RotateCoord::CheckOverlap()
{
    m_Overlap[0] = PointInSegment(m_CenterY + m_Distance, m_StartY, m_StopY - m_StartY + 1);
    m_Overlap[1] = PointInSegment(m_CenterX + m_Distance, m_StartX, m_StopX - m_StartX + 1);
    m_Overlap[2] = PointInSegment(m_CenterY - m_Distance, m_StartY, m_StopY - m_StartY + 1);
    m_Overlap[3] = PointInSegment(m_CenterX - m_Distance, m_StartX, m_StopX - m_StartX + 1);
}

bool RotateCoord::MoveToNextRound()
{
    // always pick the first valid position following current direction
    if(m_Overlap[0]){
        m_Direction = 0;
        m_CurrentX  = (std::max)(m_StartX, m_CenterX - m_Distance);
        m_CurrentY  = m_CenterY + m_Distance;
        return true;
    }else if(m_Overlap[1]){
        m_Direction = 1;
        m_CurrentX  = m_CenterX + m_Distance;
        m_CurrentY  = (std::min)(m_StopY, m_CenterY + m_Distance);
        return true;
    }else if(m_Overlap[2]){
        m_Direction = 2;
        m_CurrentX  = (std::min)(m_StopX, m_CenterX + m_Distance);
        m_CurrentY  = m_CenterY - m_Distance;
        return true;
    }else if(m_Overlap[3]){
        m_Direction = 3;
		m_CurrentX  = m_CenterX - m_Distance;
        m_CurrentY  = (std::max)(m_StartY, m_CenterY - m_Distance);
        return true;
    }else{
        return false;
    }
}

bool RotateCoord::Forward()
{
    // based on boundary but not current position
    // every time when we reach the boundary, check next boundary
    if(m_Distance > 0){
        switch(m_Direction){
            case 0:
                {
                    if(m_CurrentX == (std::min)(m_StopX, m_CenterX + m_Distance)){
                        if(m_Overlap[1]){

                            // 0 and 1 are neighbor
                            // and (NewX, NewY) is the point of next edge direction 1
                            // so need to check if overlap at the endpoint of direction 0
                            //
                            int nNewX = m_CenterX + m_Distance;
                            int nNewY = (std::min)(m_StopY, m_CenterY + m_Distance);

                            if(true
                                    && (nNewX == m_CurrentX)
                                    && (nNewY == m_CurrentY)){ nNewY--; }

                            // I didn't store the (W, H)
                            // so for W and H need to calculate it
                            if(PointInRectangle(nNewX, nNewY, m_StartX, m_StartY, m_StopX - m_StartX + 1, m_StopY - m_StartY + 1)){
                                m_Direction = 1;
                                m_CurrentX  = nNewX;
                                m_CurrentY  = nNewY;
                                return true;
                            }
                        }

                        if(m_Overlap[2]){
                            // if we need to jump from 0 to 2
                            // because they are not neighbor nothing needs to be check
                            // just jump to the first valid point

                            m_CurrentX  = (std::min)(m_StopX, m_CenterX + m_Distance);
                            m_CurrentY  = m_CenterY - m_Distance;
                            m_Direction = 2;
                            return true;
                        }

                        if(m_Overlap[3]){

                            // 0 and 3 are neighbors and if we need to jump to 3
                            // need to check whether it's overlapping with the start point of direction 0

                            // (NewX, NewY) is starting point of direction 3
                            int nNewX = m_CenterX - m_Distance;
                            int nNewY = (std::max)(m_StartY, m_CenterY - m_Distance);

                            // (D0StartX, D0StartY) is the starting point of direction 0
                            // and we have traversed it
                            int nD0StartX = (std::max)(m_StartX, m_CenterX - m_Distance);
                            int nD0StartY = m_CenterY + m_Distance;

                            if(true
                                    && (nNewX == nD0StartX)
                                    && (nNewY == nD0StartY)){
                                // ooops
                                // start point of direction 3 is the point we have tranversed
                                // do nothing here and wait to jump to next round
                            }else{
                                m_Direction = 3;
                                m_CurrentX  = nNewX;
                                m_CurrentY  = nNewY;
                                return true;
                            }
                        }

                        // can't take direction 1, 2, 3
                        // means we have done for current round
                        m_Distance++;
                        CheckOverlap();
                        return MoveToNextRound();

                    }else{
                        // move forward to next gird
                        // haven't finish direction 0 yet
                        m_CurrentX++;
                        return true;
                    }
                    break;
                }
            case 1:
                {
                    // for rest cases I won't write detailed comments
                    // refer to case-0 for reasoning

                    if(m_CurrentY == (std::max)(m_StartY, m_CenterY - m_Distance)){
                        if(m_Overlap[2]){
                            int nNewX = (std::min)(m_StopX, m_CenterX + m_Distance);
                            int nNewY = m_CenterY - m_Distance;
                            if(true
                                    && (nNewX == m_CurrentX)
                                    && (nNewY == m_CurrentY)){ nNewX--; }

                            if(PointInRectangle(nNewX, nNewY, m_StartX, m_StartY, m_StopX - m_StartX + 1, m_StopY - m_StartY + 1)){
                                m_Direction = 2;
                                m_CurrentX  = nNewX;
                                m_CurrentY  = nNewY;
                                return true;
                            }
                        }

                        if(m_Overlap[3]){
                            int nNewX = m_CenterX - m_Distance;
                            int nNewY = (std::max)(m_StartY, m_CenterY - m_Distance);

                            int nD0StartX = (std::max)(m_StartX, m_CenterX - m_Distance);
                            int nD0StartY = m_CenterY + m_Distance;

                            if(true
                                    && (nNewX == nD0StartX)
                                    && (nNewY == nD0StartY)){
                            }else{
                                m_Direction = 3;
                                m_CurrentX  = nNewX;
                                m_CurrentY  = nNewY;
                                return true;
                            }
                        }

                        m_Distance++;
                        CheckOverlap();
                        return MoveToNextRound();

                    }else{
                        m_CurrentY--;
                        return true;
                    }
                    break;
                }
            case 2:
                {
                    if(m_CurrentX == (std::max)(m_StartX, m_CenterX - m_Distance)){
                        if(m_Overlap[3]){
                            int nNewX = m_CenterX - m_Distance;
                            int nNewY = (std::max)(m_StartY, m_CenterY - m_Distance);
                            if(true
                                    && (nNewX == m_CurrentX)
                                    && (nNewY == m_CurrentY)){ nNewY++; }

                            if(PointInRectangle(nNewX, nNewY, m_StartX, m_StartY, m_StopX - m_StartX + 1, m_StopY - m_StartY + 1)){
                                int nD0StartX = (std::max)(m_StartX, m_CenterX - m_Distance);
                                int nD0StartY = m_CenterY + m_Distance;
                                if(true
                                        && (nNewX == nD0StartX)
                                        && (nNewY == nD0StartY)){
                                }else{
                                    m_Direction = 3;
                                    m_CurrentX  = nNewX;
                                    m_CurrentY  = nNewY;
                                    return true;
                                }
                            }
                        }

                        m_Distance++;
                        CheckOverlap();
                        return MoveToNextRound();
                    }else{
                        m_CurrentX--;
                        return true;
                    }
                    break;
                }
            case 3:
                {
                    // complicated part
                    // direction 3 will jump to direction 0 if finished

                    if(m_StopY >= m_CenterY + m_Distance){
                        if(m_CurrentY >= m_CenterY + m_Distance - 1){

                            // when (vertical) edge of direction 3 is enough long
                            // even if there are still valid points below current point
                            // we make a turn to start next round

                            m_Distance++;
                            CheckOverlap();
                            return MoveToNextRound();
                        }else{
                            m_CurrentY++;
                            return true;
                        }
                    }else{
                        if(m_CurrentY == m_StopY){
                            m_Distance++;
                            CheckOverlap();
                            return MoveToNextRound();
                        }else{
                            m_CurrentY++;
                            return true;
                        }
                    }
                    break;
                }
            default:
                {
                    return false;
                }
        }
    }else if(m_Distance == 0){
        // when center point is inside the rectangle
        // and we are immediately after calling Reset()
        // for this case the current X and Y are the center point
        m_Distance++;
        CheckOverlap();
        return MoveToNextRound();
    }else{
        return false;
    }
}
