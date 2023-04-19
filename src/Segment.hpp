
 #include "plugin.hpp"
#include "nanovg.h"

template <typename Derived>
struct SegmentDataInterface {
    int getSegmentStart() const
    {
        return static_cast<Derived>(this)->getSegmentStart();
    }
    int getSegmentLength() const
    {
        return static_cast<Derived>(this)->getSegmentLength();
    }
    int getSegmentMaxLength() const
    {
        return static_cast<Derived>(this)->getSegmentMaxLength();
    }
    int getActiveGate() const
    {
        return static_cast<Derived>(this)->getActiveGate();
    }
};

template<typename ContainerModule>
struct Segment2x8 : widget::Widget
{
    ContainerModule *module;
    int start = 0;
    int length = 16;
    int maxLength = 16;

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
        const int end = (start + length - 1) % maxLength;

        const int columnSize = 8;
        const int startCol = start / columnSize;
        const int endCol = end / columnSize;
        // const int startInCol = start % columnSize;
        // const int endInCol = end % columnSize;
        const int startInCol = start % 8; // % 8 is the same as %8
        const int endInCol = end % 8;

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
                // drawLine(ctx, startCol, startInCol, min((maxLength - 1) % columnSize, columnSize - 1), true, false);
                drawLine(ctx, startCol, startInCol, min((maxLength - 1) % 8, columnSize - 1), true, false);
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
                        // drawLine(ctx, !startCol, 0, min(columnSize - 1, (maxLength - 1) % columnSize), false, false);
                        drawLine(ctx, !startCol, 0, min(columnSize - 1, (maxLength - 1) % 8), false, false);
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

            drawLineSegments(args.vg, (module)->getSegmentStart(), static_cast<ContainerModule*>(module)->getSegmentLength(), static_cast<ContainerModule*>(module)->getSegmentMaxLength());
            const int activeGate = static_cast<ContainerModule*>(module)->getActiveGate();
            const int activeGateCol = activeGate / 8;
            const float activeGateX = HP + activeGateCol * 2 * HP;
            // const float activeGateY = (activeGate % 8) * JACKYSPACE; // Opt %
            const float activeGateY = (activeGate % 8) * JACKYSPACE; // % 8 is faster than %8

            // Active step
            nvgBeginPath(args.vg);
            nvgCircle(args.vg, mm2px(activeGateX), mm2px(activeGateY), 10.f);
            nvgFillColor(args.vg, rack::color::WHITE);
            nvgFill(args.vg);
        }
    }
};
template<typename ContainerModule>
Segment2x8<ContainerModule> *createSegment2x8Widget(ContainerModule *module, Vec pos, Vec size)
{
    Segment2x8<ContainerModule> *display = createWidget<Segment2x8<ContainerModule>>(pos);
    display->module = module;
    display->box.size = size;
    return display;
};