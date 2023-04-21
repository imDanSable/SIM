
#include "plugin.hpp"
#include "nanovg.h"
struct Segment2x8Data {
    int start;
    int length;
    int max;
    int active;
};
template<typename Container>
struct Segment2x8 : widget::Widget
{
    Container *module;
    std::function <Segment2x8Data()> getSegment2x8Data;

    void draw(const DrawArgs &args) override
    {
        drawLayer(args, 0);
    }

    void drawLine(NVGcontext *ctx, int startCol, int startInCol, int endInCol, bool actualStart, bool actualEnd)
    {
        Vec startVec = mm2px(Vec(HP + startCol * 2 * HP, startInCol * JACKYSPACE));
        Vec endVec = mm2px(Vec(HP + startCol * 2 * HP, endInCol * JACKYSPACE));

        if (startInCol == endInCol)
        {
            nvgFillColor(ctx, panelBlue);
            nvgBeginPath(ctx);
            nvgCircle(ctx, startVec.x, startVec.y, 12.f);
            nvgFill(ctx);
        }
        else
        {
            nvgStrokeColor(ctx, panelPink);
            nvgLineCap(ctx, NVG_ROUND);
            nvgStrokeWidth(ctx, 20.f);

            nvgBeginPath(ctx);
            nvgMoveTo(ctx, startVec.x, startVec.y);
            nvgLineTo(ctx, endVec.x, endVec.y);
            nvgStroke(ctx);
            if (actualStart)
            {
                nvgFillColor(ctx, panelBlue);
                nvgBeginPath(ctx);
                nvgCircle(ctx, startVec.x, startVec.y, 10.f);
                nvgRect(ctx, startVec.x - 10.f, startVec.y, 20.f, 10.f);
                nvgFill(ctx);
            }
            if (actualEnd)
            {
                nvgFillColor(ctx, panelBlue);
                nvgBeginPath(ctx);
                nvgCircle(ctx, endVec.x, endVec.y, 10.f);
                nvgRect(ctx, endVec.x - 10.f, endVec.y - 10.f, 20.f, 10.f);
                nvgFill(ctx);
            }
        }
    }

    void drawLineSegments(NVGcontext *ctx, int start, int length, int maxLength)
    {
        int columnSize = 8;
        int end = (start + length - 1) % maxLength;

        int startCol = start / columnSize;
        int endCol = end / columnSize;
        int startInCol = start % columnSize;
        int endInCol = end % columnSize;

        if (startCol == endCol && start <= end)
        {
            drawLine(ctx, startCol, startInCol, endInCol, true, true);
        }
        else
        {
            if (startCol == 0)
            {
                drawLine(ctx, startCol, startInCol, min(maxLength - 1, columnSize - 1), true, false);
                drawLine(ctx, endCol, 0, endInCol, false, true);
            }
            else
            {
                drawLine(ctx, startCol, startInCol, min((maxLength - 1) % columnSize, columnSize - 1), true, false);
                drawLine(ctx, endCol, 0, endInCol, false, true);
            }

            if (length > columnSize)
            {
                if (startCol == endCol)
                {
                    if ((startCol != 0) && (maxLength > columnSize))
                    {
                        drawLine(ctx, !startCol, 0, columnSize - 1, false, false);
                    }
                    else
                    {
                        drawLine(ctx, !startCol, 0, min(columnSize - 1, (maxLength - 1) % columnSize), false, false);
                    }
                }
            }
        }
    };

    void drawLayer(const DrawArgs &args, int layer) override
    {
        if (layer == 0)
        {
            if (!module)
            {
                // Draw for the browser and screenshot
                drawLineSegments(args.vg, 3, 11, 16);
                const float activeGateX = HP;
                const float activeGateY = 6 * JACKYSPACE; // XXX Opt %
                // Active step
                nvgBeginPath(args.vg);
                nvgCircle(args.vg, mm2px(activeGateX), mm2px(activeGateY), 10.f);
                nvgFillColor(args.vg, rack::color::WHITE);
                nvgFill(args.vg);
                return;
            }
            // const int editChannel = module->params[Spike::PARAM_EDIT_CHANNEL].getValue();
            // const int start = module->start[editChannel];
            // const int length = module->length[editChannel];
            // const int prevChannel = module->prevChannelIndex[editChannel];

            // const int maximum = module->inx && module->inx->inputs[editChannel].getChannels() > 0 ? module->inx->inputs[editChannel].getChannels() : 16;

            //XXX OPTIMIZE
            Segment2x8Data segmentdata = getSegment2x8Data();
            drawLineSegments(args.vg, segmentdata.start, segmentdata.length, segmentdata.max);

            // const int activeGateCol = prevChannel / 8;
            const int activeGateCol = segmentdata.active / 8;
            const float activeGateX = HP + activeGateCol * 2 * HP;
            // const float activeGateY = (prevChannel % 8) * JACKYSPACE; // XXX Opt %
            const float activeGateY = (segmentdata.active % 8) * JACKYSPACE; // XXX Opt %
            // Active step
            nvgBeginPath(args.vg);
            nvgCircle(args.vg, mm2px(activeGateX), mm2px(activeGateY), 10.f);
            nvgFillColor(args.vg, rack::color::WHITE);
            nvgFill(args.vg);
        }
    }
};

template<typename Container>
Segment2x8<Container> *createSegment2x8Widget(Container *module, Vec pos, Vec size,
    std::function<Segment2x8Data()> getSegment2x8Data)
{
    Segment2x8<Container> *display = createWidget<Segment2x8<Container>>(pos);
    display->module = module;
    display->box.size = size;
    display->getSegment2x8Data = getSegment2x8Data;
    return display;
};