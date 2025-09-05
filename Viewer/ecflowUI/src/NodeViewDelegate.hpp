/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#ifndef ecflow_viewer_NodeViewDelegate_HPP
#define ecflow_viewer_NodeViewDelegate_HPP

#include <string>

#include <QBrush>
#include <QLinearGradient>
#include <QMap>
#include <QPen>
#include <QRegularExpression>
#include <QStyledItemDelegate>

#include "FontMetrics.hpp"
#include "RectMetrics.hpp"
#include "VProperty.hpp"
#include "ViewerUtil.hpp"

class PropertyMapper;

// Base class for the node/attr renderer properties
struct BaseNodeDelegateBox
{
    BaseNodeDelegateBox() : sizeHintCache(QSize(10, 10)), selectRm(2) {}

    virtual ~BaseNodeDelegateBox() = default;

    virtual void adjust(const QFont& f) = 0;
    virtual QRect adjustTextRect(const QRect& rIn) const {
        QRect r = rIn;
        return r;
    }
    virtual QRect adjustTextBgRect(const QRect& rIn) const {
        QRect r = rIn;
        return r;
    }
    virtual QRect adjustSelectionRect(const QRect& optRect) const {
        QRect r = optRect;
        r.adjust(leftMargin, 1, -1, -1);
        return r;
    }
    virtual QRect adjustSelectionRectNonOpt(const QRect& optRect) const {
        QRect r = optRect;
        r.adjust(0, 1, -1, -1);
        return r;
    }

    int fontHeight{10};
    int height{10};
    int fullHeight{10};
    int topMargin{0};
    int bottomMargin{0};
    int leftMargin{0};
    int rightMargin{0};
    int topPadding{0};
    int bottomPadding{0};
    int leftPadding{0};
    int rightPadding{0};
    QSize sizeHintCache;
    int spacing{2};
    RectMetrics selectRm;
};

// Node renderer properties
struct NodeDelegateBox : public BaseNodeDelegateBox
{
    NodeDelegateBox() = default;

    int iconSize{16};
    int iconPreGap{2};
    int iconGap{2};

    void adjust(const QFont& f) override {
        QFontMetrics fm(f);
        fontHeight    = fm.height();
        height        = fontHeight + topPadding + bottomPadding;
        fullHeight    = height + topMargin + bottomMargin;
        sizeHintCache = QSize(100, fullHeight);
        spacing       = ViewerUtil::textWidth(fm, 'A') * 3 / 4;

        auto h   = static_cast<int>(static_cast<float>(fm.height()) * 0.7);
        iconSize = h;
        if (iconSize % 2 == 1) {
            iconSize += 1;
        }

        iconGap = 1;
        if (iconSize > 16) {
            iconGap = 2;
        }

        iconPreGap = ViewerUtil::textWidth(fm, 'A') / 2;
    }
};

// Attr renderer properties
struct AttrDelegateBox : public BaseNodeDelegateBox
{
    AttrDelegateBox() : iconSize(16), iconPreGap(2), iconGap(2) {}

    int iconSize;
    int iconPreGap;
    int iconGap;

    void adjust(const QFont& f) override {
        QFontMetrics fm(f);
        fontHeight    = fm.height();
        height        = fontHeight + topPadding + bottomPadding;
        fullHeight    = height + topMargin + bottomMargin;
        sizeHintCache = QSize(100, fullHeight);
        spacing       = ViewerUtil::textWidth(fm, 'A') * 3 / 4;

        int h    = static_cast<int>(static_cast<float>(fm.height()) * 0.7);
        iconSize = h;
        if (iconSize % 2 == 1) {
            iconSize += 1;
        }

        iconGap = 1;
        if (iconSize > 16) {
            iconGap = 2;
        }

        iconPreGap = ViewerUtil::textWidth(fm, 'A') / 2;
    }
};

class LabelStyle {
public:
    explicit LabelStyle(const std::string& prefix, bool alwaysEnabled = false);
    void update();

    bool alwaysEnabled_;
    bool enabled_;
    bool enabledBg_;
    QPen fontPen_;
    QBrush bgBrush_;
    QRegularExpression regex_;
    VProperty* enabledProp_;
    VProperty* enabledBgProp_;
    VProperty* fontProp_;
    VProperty* bgProp_;
    VProperty* regexProp_;
};

class NodeViewDelegate : public QStyledItemDelegate, public VPropertyObserver {
public:
    explicit NodeViewDelegate(QWidget* parent = nullptr);
    ~NodeViewDelegate() override;

    void notifyChange(VProperty*) override;
    void setMaxLimitItems(int v) { maxLimitItems_ = v; }

protected:
    virtual void updateSettings() = 0;
    void addBaseSettings(std::vector<std::string>&);
    void updateBaseSettings();

    void renderSelectionRect(QPainter* painter, QRect r) const;

    virtual void renderStatus(QPainter* painter, const QModelIndex& index, const QStyleOptionViewItem& option) const;

    typedef void (NodeViewDelegate::*AttributeRendererProc)(QPainter* painter,
                                                            QStringList data,
                                                            const QStyleOptionViewItem& option,
                                                            QSize&,
                                                            const QColor& foregroundColour) const;

    virtual void renderMeter(QPainter* painter,
                             QStringList data,
                             const QStyleOptionViewItem& option,
                             QSize&,
                             const QColor& foregroundColour) const;
    virtual void renderLabel(QPainter* painter,
                             QStringList data,
                             const QStyleOptionViewItem& option,
                             QSize&,
                             const QColor& foregroundColour) const;
    virtual void renderAviso(QPainter* painter,
                             QStringList data,
                             const QStyleOptionViewItem& option,
                             QSize&,
                             const QColor& foregroundColour) const;
    virtual void renderMirror(QPainter* painter,
                              QStringList data,
                              const QStyleOptionViewItem& option,
                              QSize&,
                              const QColor& foregroundColour) const;
    virtual void renderEvent(QPainter* painter,
                             QStringList data,
                             const QStyleOptionViewItem& option,
                             QSize&,
                             const QColor& foregroundColour) const;
    virtual void renderVar(QPainter* painter,
                           QStringList data,
                           const QStyleOptionViewItem& option,
                           QSize&,
                           const QColor& foregroundColour) const;
    virtual void renderGenvar(QPainter* painter,
                              QStringList data,
                              const QStyleOptionViewItem& option,
                              QSize&,
                              const QColor& foregroundColour) const;
    virtual void renderLimit(QPainter* painter,
                             QStringList data,
                             const QStyleOptionViewItem& option,
                             QSize&,
                             const QColor& foregroundColour) const;
    virtual void renderLimiter(QPainter* painter,
                               QStringList data,
                               const QStyleOptionViewItem& option,
                               QSize&,
                               const QColor& foregroundColour) const;
    virtual void renderTrigger(QPainter* painter,
                               QStringList data,
                               const QStyleOptionViewItem& option,
                               QSize&,
                               const QColor& foregroundColour) const;
    virtual void renderTime(QPainter* painter,
                            QStringList data,
                            const QStyleOptionViewItem& option,
                            QSize&,
                            const QColor& foregroundColour) const;
    virtual void renderDate(QPainter* painter,
                            QStringList data,
                            const QStyleOptionViewItem& option,
                            QSize&,
                            const QColor& foregroundColour) const;
    virtual void renderRepeat(QPainter* painter,
                              QStringList data,
                              const QStyleOptionViewItem& option,
                              QSize&,
                              const QColor& foregroundColour) const;
    virtual void renderLate(QPainter* painter,
                            QStringList data,
                            const QStyleOptionViewItem& option,
                            QSize&,
                            const QColor& foregroundColour) const;
    virtual void renderAutoArchive(QPainter* painter,
                                   QStringList data,
                                   const QStyleOptionViewItem& option,
                                   QSize&,
                                   const QColor& foregroundColour) const;
    virtual void renderAutoCancel(QPainter* painter,
                                  QStringList data,
                                  const QStyleOptionViewItem& option,
                                  QSize&,
                                  const QColor& foregroundColour) const;
    virtual void renderAutoRestore(QPainter* painter,
                                   QStringList data,
                                   const QStyleOptionViewItem& option,
                                   QSize&,
                                   const QColor& foregroundColour) const;
    virtual void renderQueue(QPainter* painter,
                             QStringList data,
                             const QStyleOptionViewItem& option,
                             QSize&,
                             const QColor& foregroundColour) const;

    void labelSize(QStringList data, int& totalWidth, int& totalHeight) const;
    int labelHeight(int) const;
    void renderVarCore(QPainter* painter, QStringList data, const QStyleOptionViewItem& option, QSize&, QColor) const;

    QPen hoverPen_;
    QBrush hoverBrush_;
    QPen selectPen_;
    QBrush selectBrush_;
    QPen nodePen_;
    QPen nodeSelectPen_;
    QPixmap errPix_;

    QBrush lostConnectBgBrush_;
    QBrush lostConnectBandBrush_;
    QBrush noConnectBgBrush_;
    QBrush noConnectBandBrush_;

    QMap<QString, AttributeRendererProc> attrRenderers_;

    PropertyMapper* prop_{nullptr};
    QFont font_;
    QFont attrFont_;
    NodeDelegateBox* nodeBox_{nullptr};
    AttrDelegateBox* attrBox_{nullptr};

    bool useStateGrad_{true};
    mutable QLinearGradient grad_;
    static int lighter_;
    bool drawAttrSelectionRect_{false};
    QBrush eventFillBrush_;
    QBrush eventBgBrush_;
    QBrush meterFillBrush_;
    QBrush meterThresholdBrush_;
    QBrush limitFillBrush_;
    QBrush limitExtraFillBrush_;
    QPixmap limitFillPix_;
    QPixmap limitEmptyPix_;
    QPixmap limitExtraFillPix_;
    enum LimitShape { RectLimitShape, CircleLimitShape };
    LimitShape limitShape_{CircleLimitShape};
    int maxLimitItems_{-1};
    QPen repeatDayPen_;
    QPen repeatDatePen_;
    QPen repeatEnumPen_;
    QPen repeatIntPen_;
    QPen repeatStringPen_;
    QBrush triggerBgBrush_;
    QPen triggerBorderPen_;
    QPen triggerFontPen_;
    QBrush completeBgBrush_;
    QPen completeBorderPen_;
    QPen completeFontPen_;
    QPen holdingTimeFontPen_;
    QPen holdingDateFontPen_;

    int avisoPixId_;
    int mirrorPixId_;
    int holdingTimePixId_;
    int holdingDatePixId_;

    enum LabelType { DefaultLabel, ErrorLabel, WarningLabel, InfoLabel };
    QMap<LabelType, LabelStyle*> labelStyle_;
};

#endif /* ecflow_viewer_NodeViewDelegate_HPP */
