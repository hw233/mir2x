/*
 * =====================================================================================
 *
 *       Filename: tokenboard.hpp
 *        Created: 06/17/2015 10:24:27
 *  Last Modified: 09/24/2017 01:02:09
 *
 *    Description: For scenarios we need text-emoticon mixed boards:
 *
 *                 1. button like text terminal. which we only need to support emoticon
 *                    and eventtext, eventtext accept button click event only. This is
 *                    the simple case. no select/edit support needed.
 *
 *                 2. chatbox, two parts:
 *                      a. already sent chat messages.
 *                      b. editting chatting messages.
 *
 *                      +--------------------------+ 
 *                      | xxxxx :), xxxxxxxxxxxxxx |
 *                      | xxxxxxxxxxxxxxxxxxx.     |
 *                      | xx                       | <----- already sent messages
 *                      | xxxxxxxxxxxxxxxxxxxxxxxx |
 *                      | xxxxxxxxxxxxxxxxxxxxxxxx |
 *                      | xxxxxxxxxxxxxxxxxxxxxxxx |
 *                      +--------------------------+
 *                      | xxx :( xxxx |            |
 *                      |                          | <----- editting messages
 *                      +--------------------------+
 *
 *                      for 2-a: there is emoticon, normal text, eventtext, eventtext for hyperlinks
 *                               to trigger event, just like 1. 2-a should support select.
 *
 *                      for 2-b: there is emoticon and normal text. need to support select/edit. as
 *                               analyzed, edit is insert, and insert can be implemented by select.
 *                               so 2-b we need to support select.
 *                   
 *                 Event for handling or not:
 *
 *                 1. click, motion, for sure we need to handle it for trigger event, for selection.
 *                 2. ctrl+c/p/x, a/b/c..., Tokenboard won't handle this. alternatively, wrapper of
 *                    tokenboard handle it and query TokenBoard for proper behavor. IE:
 *                          ChatBox get event:
 *                          if it's "a":
 *                             1. if there is selected part, replace this part with ``a"
 *                             2. if not, put ``a" before the cursor.
 *                          if it's click:
 *                             1. directly pass it to tokenboard, to set cursor.
 *                          if it's ctrl-x:
 *                             1. query the tokenboard, if there is selected part, cut it.
 *                             2. otherwise do nothing.
 *
 *                 Some thing to think about:
 *
 *                 1. what's needed?
 *
 *                      What we need is to have class which can support case-1, case-2(a) and case-
 *                      2-(b), for case-1, we need support click event trigger, and the board is as
 *                      non-state class. For case-2(a) we need to support click event trigger, sele-
 *                      ct. For case-2(b) we need to support select, insert.
 *
 *                 2. how to support it?
 *                      Case-1 is simple and already supported. For select, we need to locate the
 *                      token under pointer. For insert we need to support builtin cursor. Both need
 *                      a (int, int) to support. Use (x, y) or (section, offset)?
 *
 *                      Let's use (x, y), since (x, y)->section is quick, but (section, offset)->
 *                      (x, y) is slow.
 *
 *                 3. let me add a (bSelectable, bEditable) to the class, when disable both, we have
 *                    the button-like text-terminal.
 *
 *                    (bSelectable, bEditable) with value:
 *
 *                    1. (0, 0): basic button-like text-terminal
 *                    2. (0, 1): no idea of this mode
 *                    3. (1, 0): sent-message box
 *                    4. (1, 1): input box
 *
 *                    or use (bSelectable, bWithCursor) with value:
 *                    1. (0, 0): basic button-like text-terminal
 *                    2. (0, 1): can't select, but can put cursor everywhere and insert, when push
 *                               and moving mouse, the cursor moving respectively
 *                    3. (1, 0): sent-message box, no cursor shown
 *                    4. (1, 1): classical input box
 *
 *                    Let's use (bSelectable, bWithCursor), having cursor means editable.
 *
 *                 TODO
 *                 about background color:
 *
 *                 I need to add background color support
 *                 background color can be used for selection and highlight
 *                 but I decide to not include this background information in section
 *                      1. different selection should not alternate the board status
 *                      2. we can select multiple sections
 *                      3. selection changes lot but board status shouldn't
 *                 the reason-2 means we need new information indepdent from section
 *
 *                 I am thinking of implement background color support as two types:
 *                 1. static background color
 *                 2. dynamic
 *
 *                 static background color can be initialized in XML as
 *                      <OBJECT TYPE="PLAINTEXT" BACKCOLOR="RED">xxx</OBJ>
 *                 dynamic can use SelectBox() to specify
 *
 *
 *
 *                    Add another flag: bCanThrough, if true
 *
 *
 *                    +-----+  +---+
 *                    |     |  |   |
 *                    |     |  |   |  +----+
 *                L1  +-----+--+   +- |    |  <--- this is can through
 *                             |   |  |    |
 *                             +---+  |    |
 *                           +-----+  |    |
 *                           |     |  |    |
 *                           |     |  |    |
 *                L2  -------+-----+--+----+
 *
 *
 *                    if bCanThrough = false:
 *
 *                    +-----+  +---+
 *                    |     |  |   |
 *                    |     |  |   | 
 *                 L1 +-----+--+   +---------
 *                             |   |
 *                             +---+
 *                                    +----+  <--- this is can't through
 *                                    |    |
 *                                    |    |
 *                           +-----+  |    |
 *                           |     |  |    |
 *                           |     |  |    |
 *                 L2 -------+-----+--+----+
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
#pragma once

#include <map>
#include <limits>
#include <vector>
#include <SDL2/SDL.h>
#include <tinyxml2.h>
#include <functional>
#include <unordered_map>

#include "widget.hpp"
#include "section.hpp"
#include "tokenbox.hpp"
#include "xmlobjectlist.hpp"

class TokenBoard: public Widget
{
    // +-----------------------------+
    // |           0                 |
    // |  +---------------------+    |
    // |  |XXXXXXXX:)           |    |
    // |3 |XXXXXXXXXXXXXXX      | 1  |
    // |  |          XX         |    |
    // |  +---------------------+    |
    // |                             |
    // |           2                 |
    // +-----------------------------+

    // 1. marg0 ~ marg3 is for cursor blitting
    // 2. Internal box is for tokenbox *only*, |X X X  X|, no extra space
    //    any more in it beside the boundary
    // 3. The outer box as a whole to accept events

    private:
        enum SelectType: int
        {
            SELECTTYPE_NONE      = 0,
            SELECTTYPE_SELECTING = 1,
            SELECTTYPE_DONE      = 2,
        };

    private:
        struct TBLocation
        {
            int X;
            int Y;

            TBLocation(int nX = -1, int nY = -1)
                : X(nX)
                , Y(nY)
            {}
        };

        struct SelectRecord
        {
            int X0;
            int Y0;
            int X1;
            int Y1;

            // [0] : font color
            // [1] : back ground color
            SDL_Color Color[2];
        };

        struct TextLine
        {
            int StartY;
            bool EndWithCR;
            std::vector<TOKENBOX> Content;
        };

        struct SectionEntry
        {
            SECTION Section;
            std::function<void()> Callback;
        };

    private:
        using IDHandlerMap = std::unordered_map<std::string, std::function<void()>>;

    private:
        bool m_Selectable;
        bool m_WithCursor;
        bool m_SpacePadding;
        bool m_CanThrough;

    private:
        // define max line width in pixel
        // <= 0 : not fixed
        // >  0 :
        int m_MaxLineWidth;

    private:
        int m_WordSpace;
        int m_LineSpace;

    private:
        uint8_t   m_DefaultFont;
        uint8_t   m_DefaultSize;
        uint8_t   m_DefaultStyle;
        SDL_Color m_DefaultColor;

    private:
        // margins for the board, this is needed for balabala..
        //  +-------------------------+
        //  |        M0               |
        //  |  +-------------+        |
        //  |  |             |        |
        //  |M3|             |  M1    |
        //  |  |             |        |
        //  |  +-------------+        |
        //  |        M2               |
        //  +-------------------------+
        int m_Margin[4];

    private:
        std::vector<TextLine> m_LineV;

    private:
        std::map<int, SectionEntry> m_SectionRecord;

    private:
        bool m_SkipEvent;
        bool m_SkipUpdate;

    private:
        // keep it mutable and dynamically update it
        // define the grid size assistant to token box selecting
        int m_Resolution;

        // token box map
        // record box's location rather than its pointer
        std::vector<std::vector<std::vector<TBLocation>>> m_TokenBoxBitmap;

    private:
        // cursor binds to the token box after it
        // (0,              Y) : before the first token box
        // (Content.size(), Y) : after  the last  token box
        TBLocation m_CursorLoc;

        // last token box selected
        // location for token box, not for cursor
        TBLocation m_LastTokenBoxLoc;

    private:
        int        m_SelectState;
        TBLocation m_SelectLoc[2];

    private:
        std::map<int, SelectRecord> m_SelectRecord;

    public:
        // parameters
        //
        // bSelectable:       can select and copy
        // bWithCursor:       enable with: 1. can insert before cursor
        //                                 2. show a cursor
        //                                 3. select has nothing to do with current cursor
        // nMaxLineWidth:     positive for wrap, non-positive for no-wrap
        // nWordSpace
        // nLineSpace

        TokenBoard(
                int nX,
                int nY,

                bool bSelectable,
                bool bWithCursor,
                bool bSpacePadding,
                bool bCanThrough,

                int nMaxLineWidth = -1,

                int nWordSpace = 0,
                int nLineSpace = 0,

                uint8_t          nDefaultFont    =  0,
                uint8_t          nDefaultSize    =  10,
                uint8_t          nDefaultStyle   =  0,
                const SDL_Color &rstDefaultColor =  {0XFF, 0XFF, 0XFF, 0XFF},

                int nMargin0 = 0,
                int nMargin1 = 0,
                int nMargin2 = 0,
                int nMargin3 = 0,

                Widget *pWidget     = nullptr,
                bool    bFreeWidget = false)
            : Widget(nX, nY, 0, 0, pWidget, bFreeWidget)

            , m_Selectable(bSelectable)
            , m_WithCursor(bWithCursor)
            , m_SpacePadding(bSpacePadding)
            , m_CanThrough(bCanThrough)

            , m_MaxLineWidth(nMaxLineWidth)

            , m_WordSpace(nWordSpace)
            , m_LineSpace(nLineSpace)

            , m_DefaultFont(nDefaultFont)
            , m_DefaultSize(nDefaultSize)
            , m_DefaultStyle(nDefaultStyle)
            , m_DefaultColor(rstDefaultColor)

            , m_Margin {nMargin0, nMargin1, nMargin2, nMargin3}

            , m_LineV()
            , m_SectionRecord()

            , m_SkipEvent(false)
            , m_SkipUpdate(false)

            , m_Resolution(20)
            , m_TokenBoxBitmap()

            , m_CursorLoc(0, 0)
            , m_LastTokenBoxLoc(-1, -1)

            , m_SelectState(SELECTTYPE_DONE)
            , m_SelectLoc {{-1, -1}, {-1, -1}}
            , m_SelectRecord()
        {
            Reset();
        }

    public:
        virtual ~TokenBoard() = default;

    public:
        bool ProcessEvent(const SDL_Event &, bool *);

    public:
        void Reset();

    public:
        void Update(double);

    public:
        bool Empty(bool) const;

    private:
        void SetTokenBoxStartX(int);
        void SetTokenBoxStartY(int, int);

    public:
        int LinePadding(int);
        int DoLinePadding(int, int, int);

    public:
        void ResetLine(int);

    private:
        int TokenBoxType(const TOKENBOX &) const;

    public:
        // load by XMLObjectList, failed then the tokenboard is undefined
        // 1. previous content will be destroyed
        // 2. failed then board is undefined
        // 3. TBD:
        //      if m_WithCursor is false, then last empty line will be deleted
        bool Load(XMLObjectList &rstXMLObjectList, const IDHandlerMap &rstMap = IDHandlerMap())
        {
            Reset();
            rstXMLObjectList.Reset();
            bool bRes = InnInsert(rstXMLObjectList, rstMap);
            if(bRes && !m_WithCursor){
                DeleteEmptyBottomLine();
            }
            return bRes;
        }

        bool LoadXML(const char *szXML, const IDHandlerMap &rstMap = IDHandlerMap())
        {
            XMLObjectList stXMLObjectList;
            if(stXMLObjectList.Parse(szXML, true)){
                stXMLObjectList.Reset();
                return Load(stXMLObjectList, rstMap);
            }
            return false;
        }

    private:
        bool ParseReturnObject();
        bool ParseEmoticonObject(const tinyxml2::XMLElement &);
        bool ParseTextObject(const tinyxml2::XMLElement &, int, const std::unordered_map<std::string, std::function<void()>> &);

    private:
        bool GetAttributeColor(SDL_Color *, const SDL_Color &, const tinyxml2::XMLElement &, const std::vector<std::string> &);
        bool GetAttributeAtoi(int *, int, const tinyxml2::XMLElement &, const std::vector<std::string> &);

    private:
        bool MakeTokenBox(int, uint32_t, TOKENBOX *);

    public:
        void Draw(int nX, int nY)
        {
            DrawEx(nX, nY, 0, 0, W(), H());
        }

    public:
        void DrawEx(int, int, int, int, int, int);

    private:
        int GetNewLineStartY(int);
        int GetLineIntervalMaxH2(int, int, int);
        int GetLineTokenBoxStartY(int, int, int, int);

    private:
        void MakeTokenBoxEventBitmap();
        void MarkTokenBoxEventBitmap(int, int);

    private:
        int SectionTypeCount(int, int);

    private:
        int GuessResoltion();

    private:
        void TokenBoxGetMouseButtonUp(int, int, bool);
        void TokenBoxGetMouseButtonDown(int, int, bool);
        void TokenBoxGetMouseMotion(int, int, bool);

    public:
        bool Delete(bool);
        void DeleteEmptyBottomLine();

    public:
        bool AddTokenBoxLine(const std::vector<TOKENBOX> &);

    public:
        void GetCursor(int *pX, int *pY) const
        {
            if(pX){ *pX = m_CursorLoc.X; }
            if(pY){ *pY = m_CursorLoc.Y; }
        }

    public:
        bool SetCursor(int nX, int nY)
        {
            if(CursorValid(nX, nY)){
                m_CursorLoc = {nX, nY};
                return true;
            }
            return false;
        }

    public:
        int GetWordSpace() const
        {
            return m_WordSpace;
        }

        int GetLineSpace() const
        {
            return m_LineSpace;
        }

    public:
        int GetLineTokenBoxCount(int nLine) const
        {
            return LineValid(nLine) ? (int)(m_LineV[nLine].Content.size()) : -1;
        }

        int GetLineCount()
        {
            return (int)(m_LineV.size());
        }

        bool BreakLine();

        void SetDefaultFont(uint8_t nFont, uint8_t nSize, uint8_t nStyle, const SDL_Color &rstColor)
        {
            m_DefaultFont  = nFont;
            m_DefaultSize  = nSize;
            m_DefaultStyle = nStyle;
            m_DefaultColor = rstColor;
        }

    private:
        bool InnInsert(XMLObjectList &, const std::unordered_map<std::string, std::function<void()>> &);

    public:
        int LineFullWidth(int) const;
        int LineRawWidth(int, bool);
        int SetTokenBoxWordSpace(int);

    public:
        void MoveCursorFront()
        {
            if(m_LineV.empty()){ Reset(); }
            m_CursorLoc = {0, 0};
        }

        void MoveCursorBack()
        {
            if(m_LineV.empty()){ Reset(); }
            m_CursorLoc = {(int)(m_LineV.back().Content.size()), (int)(m_LineV.size()) - 1};
        }

    private:
        bool CursorValid(int nX, int nY) const
        {
            return true
                && nY >= 0
                && nY <  (int)(m_LineV.size())
                && nX >= 0
                && nX <= (int)(m_LineV[nY].Content.size());
        }

        bool CursorValid() const
        {
            return CursorValid(m_CursorLoc.X, m_CursorLoc.Y);
        }

        bool LineValid(int nLine) const
        {
            return nLine >= 0 && nLine < (int)(m_LineV.size());
        }

        bool TokenBoxValid(int nX, int nY) const
        {
            return true
                && nY >= 0
                && nY < (int)(m_LineV.size())
                && nX >= 0
                && nX < (int)(m_LineV[nY].Content.size());
        }

    private:
        bool LastTokenBoxValid() const
        {
            return TokenBoxValid(m_LastTokenBoxLoc.X, m_LastTokenBoxLoc.Y);
        }

    private:
        int GetNewHBasedOnLastLine();

    public:
        int GetBlankLineHeight();

    public:
        std::string GetXML(bool);
        std::string InnGetXML(int, int, int, int);

        bool QueryTokenBox(int, int, int *, int *, int *, int *, int *, int *, int *);
        void QueryDefaultFont(uint8_t *, uint8_t *, uint8_t *);

    public:
        void ResetOneLine(int);
        void ResetLineStartY(int);

    public:
        int GetLineMaxH1(int);
        int GetLineStartY(int);

    private:
        bool SectionValid(int nSectionID, bool bCheckSectionType = true) const
        {
            if(nSectionID > 0){
                auto pRecord = m_SectionRecord.find(nSectionID);
                if(pRecord != m_SectionRecord.end()){
                    if(bCheckSectionType){
                        switch(pRecord->second.Section.Info.Type){
                            case SECTIONTYPE_PLAINTEXT:
                            case SECTIONTYPE_EVENTTEXT:
                            case SECTIONTYPE_EMOTICON:
                                {
                                    return true;
                                }
                            default:
                                {
                                    return false;
                                }
                        }
                    }else{
                        return true;
                    }
                }
            }
            return false;
        }

        int CreateSection(const SECTION &rstSection, const std::function<void()> &fnCallback = [](){})
        {
            auto fnDoCreate = [this](int nSection, const SECTION &rstSEC, const std::function<void()> &fnCB)
            {
                m_SectionRecord[nSection].Section  = rstSEC;
                m_SectionRecord[nSection].Callback = fnCB;
            };

            int nSection = m_SectionRecord.empty() ? 1 : m_SectionRecord.rbegin()->first;
            fnDoCreate(nSection, rstSection, fnCallback);
            return nSection;
        }

    public:
        int SelectBox(int, int, int, int, const SDL_Color &, const SDL_Color &);

    public:
        std::string Print   (bool);
        std::string PrintXML(bool);

    public:
        int Margin(int nIndex) const
        {
            return (nIndex >= 0 && nIndex < 4) ? m_Margin[nIndex] : -1;
        }

    public:
        bool ParseXML(const char *, const std::unordered_map<std::string, std::function<void()>> &);

    public:
        // add char/string before current cursor
        // if current place is a text section then merge to it
        bool AddUTF8Code(uint32_t);
        bool AddUTF8Text(const char *);

    public:
        // create a new section of current content
        // append string/xml at the end of current board
        bool Append   (const char *);
        bool AppendXML(const char *, const std::unordered_map<std::string, std::function<void()>> &);
};
