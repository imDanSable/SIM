#pragma once
#include <cassert>
#include <rack.hpp>
#include "constants.hpp"

using namespace rack;        // NOLINT
using namespace dimensions;  // NOLINT

struct Segment2x8Data {
    int start;
    int length;
    int max;
    int active;
};
template <typename Container>
struct Segment2x8 : widget::Widget {
    template <typename T>
    friend Segment2x8<T>* createSegment2x8Widget(T* module,
                                                 Vec pos,
                                                 Vec size,
                                                 std::function<Segment2x8Data()> getSegment2x8Data);

    void draw(const DrawArgs& args) override
    {
        drawLayer(args, 0);
    }

    void drawLine(NVGcontext* ctx,
                  int startCol,
                  int startInCol,
                  int endInCol,
                  bool actualStart,
                  bool actualEnd)
    {
        Vec startVec = mm2px(Vec(HP + startCol * 2 * HP, startInCol * JACKYSPACE));  // NOLINT
        Vec endVec = mm2px(Vec(HP + startCol * 2 * HP, endInCol * JACKYSPACE));      // NOLINT

        if (startInCol == endInCol) {
            nvgFillColor(ctx, endColor);
            nvgBeginPath(ctx);
            nvgCircle(ctx, startVec.x, startVec.y, 12.F);
            nvgFill(ctx);
        }
        else {
            nvgStrokeColor(ctx, lineColor);
            nvgLineCap(ctx, NVG_ROUND);
            nvgStrokeWidth(ctx, 20.F);

            nvgBeginPath(ctx);
            nvgMoveTo(ctx, startVec.x, startVec.y);
            nvgLineTo(ctx, endVec.x, endVec.y);
            nvgStroke(ctx);
            if (actualStart) {
                nvgFillColor(ctx, endColor);
                nvgBeginPath(ctx);
                nvgCircle(ctx, startVec.x, startVec.y, 10.F);
                nvgRect(ctx, startVec.x - 10.F, startVec.y, 20.F, 10.F);
                nvgFill(ctx);
            }
            if (actualEnd) {
                nvgFillColor(ctx, endColor);
                nvgBeginPath(ctx);
                nvgCircle(ctx, endVec.x, endVec.y, 10.F);
                nvgRect(ctx, endVec.x - 10.F, endVec.y - 10.F, 20.F, 10.F);
                nvgFill(ctx);
            }
        }
    }

    void drawLineSegments(NVGcontext* ctx, const Segment2x8Data& segmentData)
    {
        assert(segmentData.start >= 0);
        assert(segmentData.start < segmentData.max);
        assert(segmentData.max <= constants::MAX_GATES);
        assert(segmentData.length <= constants::MAX_GATES);
        constexpr int columnSize = 8;
        const int end = (segmentData.start + segmentData.length - 1) % segmentData.max;

        const int startCol = segmentData.start / columnSize;
        const int endCol = end / columnSize;
        const int startInCol = segmentData.start & 7;  // NOLINT
        const int endInCol = end & 7;                  // NOLINT

        if (startCol == endCol && segmentData.start <= end) {
            drawLine(ctx, startCol, startInCol, endInCol, true, true);
        }
        else {
            if (startCol == 0) {
                drawLine(ctx, startCol, startInCol, std::min(segmentData.max - 1, columnSize - 1),
                         true, false);
                drawLine(ctx, endCol, 0, endInCol, false, true);
            }
            else {
                drawLine(ctx, startCol, startInCol,
                         std::min((segmentData.max - 1) % columnSize, columnSize - 1), true, false);
                drawLine(ctx, endCol, 0, endInCol, false, true);
            }

            if (segmentData.length > columnSize) {
                if (startCol == endCol) {
                    if ((startCol != 0) && (segmentData.max > columnSize)) {
                        drawLine(ctx, !startCol, 0, columnSize - 1, false, false);
                    }
                    else {
                        drawLine(ctx, !startCol, 0,
                                 std::min(columnSize - 1, (segmentData.max - 1) % columnSize),
                                 false, false);
                    }
                }
            }
        }
    };

    void drawLayer(const DrawArgs& args, int layer) override
    {
        if (layer == 0) {
            if (!module) {
                // Draw for the browser and screenshot
                drawLineSegments(args.vg, Segment2x8Data{3, 11, 16, 3});
                const float activeGateX = HP;
                const float activeGateY = 6 * JACKYSPACE;
                // Active step
                nvgBeginPath(args.vg);
                nvgCircle(args.vg, mm2px(activeGateX), mm2px(activeGateY), 10.F);
                nvgFillColor(args.vg, rack::color::WHITE);
                nvgFill(args.vg);
                return;
            }
            const Segment2x8Data segmentdata = getSegment2x8Data();
            drawLineSegments(args.vg, segmentdata);

            // Active step
            if (segmentdata.active >= 0) {
                assert(segmentdata.active < constants::MAX_GATES);
                const int activeGateCol = segmentdata.active / 8;
                const float activeGateX = HP + activeGateCol * 2 * HP;            // NOLINT
                const float activeGateY = (segmentdata.active & 7) * JACKYSPACE;  // NOLINT
                nvgBeginPath(args.vg);
                nvgCircle(args.vg, mm2px(activeGateX), mm2px(activeGateY), 10.F);
                nvgFillColor(args.vg, rack::color::WHITE);
                nvgFill(args.vg);
            }
        }
    }

    Container* module;                                  // NOLINT
    std::function<Segment2x8Data()> getSegment2x8Data;  // NOLINT

    NVGcolor getEndColor() const
    {
        return endColor;
    }
    void setEndColor(NVGcolor color)
    {
        endColor = color;
    }
    NVGcolor getLineColor() const
    {
        return lineColor;
    }
    void setLineColor(NVGcolor color)
    {
        lineColor = color;
    }

   private:
    NVGcolor endColor = colors::panelYellow;
    NVGcolor lineColor = colors::panelYellow;
    // Setup draw colors for themes
};

template <typename Container>
Segment2x8<Container>* createSegment2x8Widget(
    Container* module,
    Vec pos,
    Vec size,
    const std::function<Segment2x8Data()>& getSegment2x8Data)
{
    Segment2x8<Container>* display = createWidget<Segment2x8<Container>>(pos);
    display->module = module;
    display->box.size = size;
    display->getSegment2x8Data = getSegment2x8Data;

    display->setLineColor(nvgRGB(100, 100, 100));
    display->setEndColor(colors::panelYellow);

    return display;
};
