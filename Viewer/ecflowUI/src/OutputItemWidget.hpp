//============================================================================
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef OUTPUTITEMWIDGET_HPP_
#define OUTPUTITEMWIDGET_HPP_

#include "InfoPanelItem.hpp"

#include "VFile.hpp"
#include "VDir.hpp"

#include "ui_OutputItemWidget.h"

class OutputDirProvider;
class OutputFileFetchInfo;
class OutputDirWidget;
class VProperty;
class QTimer;


class OutputItemWidget : public QWidget, public InfoPanelItem, protected Ui::OutputItemWidget
{
Q_OBJECT

public:
	explicit OutputItemWidget(QWidget *parent=nullptr);
	~OutputItemWidget() override;

	void reload(VInfo_ptr) override;
	QWidget* realWidget() override;
	void clearContents() override;

	//From VInfoPresenter
	void infoReady(VReply*) override;
	void infoFailed(VReply*) override;
	void infoProgress(VReply*) override;
    void infoProgressStart(const std::string& text,int max) override;
    void infoProgressUpdate(const std::string& text,int value) override;
    void infoProgressStop() override;

    void nodeChanged(const VNode*, const std::vector<ecf::Aspect::Type>&) override;
    void defsChanged(const std::vector<ecf::Aspect::Type>&) override {}

protected Q_SLOTS:
    void slotDirItemSelected();
    void slotUpdateDirs();
    void slotShowDir(bool);
    void adjustShowDirTb();
    void shrinkDirPanel();
	void on_searchTb__clicked();
	void on_gotoLineTb__clicked();
	void on_reloadTb__clicked();
	void on_fontSizeUpTb__clicked();
	void on_fontSizeDownTb__clicked();
    void on_toStartTb__clicked();
    void on_toEndTb__clicked();
    void on_toLineStartTb__clicked();
    void on_toLineEndTb__clicked();
    void slotSaveFileAs();
    void slotCopyPath();
    void slotLineNumber(bool st);
    void on_wordWrapTb__clicked(bool st);
    void on_expandFileInfoTb__clicked(bool st);
    void slotWordWrapSupportChanged(bool);
    void slotLoadWholeFile();

protected:
    void updateState(const FlagSet<ChangeFlag>&) override;
	void searchOnReload();
    void reloadCurrentFile(bool wholeFile);
    void loadCurrentDirItemFile();
    void loadCurrentJobout();
    bool isJoboutLoaded() const;
    void updateHistoryLabel(const std::vector<std::string>&);

    OutputDirProvider* dirProvider_{nullptr};

	bool userClickedReload_{false};
    OutputFileFetchInfo* fetchInfo_{nullptr};
    bool submittedWarning_{false};
    VProperty* showDirProp_{nullptr};
    VProperty* lineNumProp_{nullptr};
    VProperty* wordWrapProp_{nullptr};
    VProperty* expandFileInfoProp_{nullptr};
};

#endif

