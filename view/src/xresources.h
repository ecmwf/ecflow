/*=============================================================================================*/
/* Name        :                                                                               */
/* Author      :                                                                               */
/* Revision    : $Revision: #18 $                                                                    */
/*                                                                                             */
/* Copyright 2009-2012 ECMWF.                                                                  */
/* This software is licensed under the terms of the Apache Licence version 2.0                 */
/* which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.                        */
/* In applying this licence, ECMWF does not waive the privileges and immunities                */
/* granted to it by virtue of its status as an intergovernmental organisation                  */
/* nor does it submit to any jurisdiction.                                                     */
/*                                                                                             */
/* Description :                                                                               */
/*=============================================================================================*/

#ifndef BRIDGE

static char* xresources = (char*) "\n"
"ecFlowview*File.accelerator: Ctrl<Key>f\n"
"ecFlowview*File.mnemonic: F\n"
"ecFlowview*Info.accelerator: Ctrl<Key>I\n"
"ecFlowview*Script.accelerator: Ctrl<Key>S\n"
"ecFlowview*Manual.accelerator: Ctrl<Key>M\n"
"ecFlowview*Jobstatus.accelerator: Ctrl<Key>j\n"
"ecFlowview*Job.accelerator: Ctrl<Key>J\n"
"ecFlowview*Triggers.accelerator: Ctrl<Key>T\n"
"ecFlowview*Why?.accelerator: Ctrl<Key>y\n"
"ecFlowview*Variables.accelerator: Ctrl<Key>V\n"
"ecFlowview*Messages.accelerator: Ctrl<Key>M\n"
"ecFlowview*Edit.accelerator: Ctrl<Key>E\n"
"ecFlowview*Search.accelerator: Ctrl<Key>c\n"
"ecFlowview*Status.accelerator: Space<Key>\n"
"ecFlowview*Login.accelerator: Ctrl<Key>L\n"
"ecFlowview*Login.acceleratorText: Ctrl+L\n"
"ecFlowview*Login.mnemonic: L\n"
"ecFlowview*Login.title: Login...\n"
"ecFlowview*Quit.accelerator: Ctrl<Key>Q\n"
"ecFlowview*Quit.acceleratorText: Ctrl+Q\n"
"ecFlowview*Quit.mnemonic: Q\n"
"ecFlowview*Edit.accelerator: Ctrl<Key>E\n"
"ecFlowview*Edit.mnemonic: E\n"
"ecFlowview*pref.accelerator: Ctrl<Key>e\n"
"ecFlowview*pref.mnemonic: e\n"
"ecFlowview*pref.title: User preferences...\n"

"ecFlowview*pref_shell.title: Preferences\n"

"ecFlowview*Show.accelerator: Ctrl<Key>S\n"
"ecFlowview*Show.mnemonic: S\n"
"ecFlowview*Servers.accelerator: Ctrl<Key>v\n"
"ecFlowview*Servers.mnemonic: v\n"
"ecFlowview*Windows.accelerator: Ctrl<Key>W\n"
"ecFlowview*Windows.mnemonic: W\n"
"ecFlowview*Help.accelerator: Ctrl<Key>H\n"
"ecFlowview*Help.mnemonic: H\n"

"ecFlowview*Version.accelerator: Ctrl<Key>V\n"
"ecFlowview*Version.mnemonic: V\n"

"ecFlowview*file_menu.labelString: File\n"
"ecFlowview*file_menu.mnemonic: F\n"
"ecFlowview*Option.mnemonic: O\n"
"ecFlowview*Print*.mnemonic: P\n"
"ecFlowview*Save*.mnemonic: S\n"
"ecFlowview*close.mnemonic: C\n"
"ecFlowview*help_menu.mnemonic: H\n"

"ecFlowview*snapshot.accelerator: Ctrl<Key>t\n"
"ecFlowview*snapshot.labelString: Snapshot\n"

"ecFlowview*file_menu.title: File\n"
"ecFlowview*file_menu.accelerator: Ctrl<Key>F\n"

"ecFlowview*options_menu.labelString: Options\n"
"ecFlowview*options_menu.mnemonic: O\n"

"ecFlowview.*SimpleBase.baseTranslations: #augment \
 Shift<Btn5Down>: increment(-1)  \\n\
 Shift<Btn4Down>: increment(1)   \\n\
      <Btn5Down>: increment(-10) \\n\
      <Btn4Down>: increment(10)  \n"

"ecFlowview.*Hyper.baseTranslations: #augment \
 Shift<Btn5Down>: increment(-1)  \\n\
 Shift<Btn4Down>: increment(1)   \\n\
      <Btn5Down>: increment(-10) \\n\
      <Btn4Down>: increment(10)  \n"

"ecFlowview.*XmList.baseTranslations: #augment \
 Shift<Btn5Down>: ListNextPage() \\n\
 Shift<Btn4Down>: ListPrevPage() \\n\
      <Btn5Down>: ListNextItem() \\n\
      <Btn4Down>: ListPrevItem() \n"

"ecFlowview.*XmScrollBar.baseTranslations: #augment \
 Shift<Btn5Down>: IncrementDownOrRight(0) IncrementDownOrRight(1) \\n\
 Shift<Btn4Down>: IncrementUpOrLeft(0) IncrementUpOrLeft(1) \\n\
      <Btn5Down>: PageDownOrRight(0) PageDownOrRight(1) \\n\
      <Btn4Down>: PageUpOrLeft(0) PageUpOrLeft(1) \n"

"ecFlowview.*XmText.baseTranslations: #augment    \
      <Key>osfUp:      scroll-one-line-up()\\n\
      <Key>osfDown:    scroll-one-line-down()\\n\
      <KeyUp>Prior:    previous-page()\\n\
      <KeyUp>Next:     next-page()\\n\
      <Key>Up:         scroll-one-line-up()   \\n\
      <Key>Down:       scroll-one-line-down() \\n\
      <KeyUp>KP_Prior: previous-page()\\n\
      <KeyUp>KP_Next:  next-page()\\n\
      <KeyUp>KP_Up:    scroll-one-line-up()   \\n\
      <KeyUp>KP_Down:  scroll-one-line-down() \n\
 Shift<Btn5Down>: previous-page()   \\n\
 Shift<Btn4Down>: next-page() \\n\
      <Btn5Down>: scroll-one-line-up()   \\n\
      <Btn4Down>: scroll-one-line-down() \n"

"ecFlowview.*DrawingAreaInput.baseTranslations: #augment    \
      <Key>osfUp:      scroll-one-line-up()\\n\
      <Key>osfDown:    scroll-one-line-down()\\n\
      <KeyUp>Prior:    previous-page()\\n\
      <KeyUp>Next:     next-page()\\n\
      <Key>Up:         scroll-one-line-up()   \\n\
      <Key>Down:       scroll-one-line-down() \\n\
      <KeyUp>KP_Prior: previous-page()\\n\
      <KeyUp>KP_Next:  next-page()\\n\
      <KeyUp>KP_Up:    scroll-one-line-up()   \\n\
      <KeyUp>KP_Down:  scroll-one-line-down() \n\
 Shift<Btn5Down>: previous-page()   \\n\
 Shift<Btn4Down>: next-page() \\n\
      <Btn5Down>: scroll-one-line-up()   \\n\
      <Btn4Down>: scroll-one-line-down() \n"

"*XmSpinBox.accelerators: #augment \
      <Btn5Down>: SpinBPrior()\\n\
      <Btn5Up>:SpinBDisarm()\\n\
      <Btn4Down>: SpinBNext()\\n\
      <Btn4Up>: SpinBDisarm()\\n\
      <Key>osfUp:SpinBNext()\\n\
      <Key>osfDown: SpinBPrior()\\n\
      <KeyUp>osfUp: SpinBDisarm()\\n\
      <KeyUp>osfDown: SpinBDisarm()\\n\
      <Key>osfLeft:  SpinBLeft()\\n\
      <Key>osfRight: SpinBRight()\\n\
      <KeyUp>osfLeft:  SpinBDisarm()\\n\
      <KeyUp>osfRight: SpinBDisarm()\\n\
      <Key>osfBeginLine: SpinBFirst()\\n\
      <Key>osfEndLine: SpinBLast()\n"

"ecFlowview*@zombied.labelString: Use default settings\n"
"ecFlowview*@aliases.labelString: Use default settings\n"
"ecFlowview*zombied.labelString: Zombies\n"
"ecFlowview*aliases.labelString: Aborted or restarted aliases\n"

"ecFlowview*File.labelString: File\n"
"ecFlowview*.XmText.background: OldLace\n"
"ecFlowview*.XmTextField.background: OldLace\n"
"ecFlowview*.scrollBarDisplayPolicy: STATIC\n"
"ecFlowview*@aborted.labelString: Use default settings\n"
"ecFlowview*@color_aborted.labelString: Use default settings\n"
"ecFlowview*@color_active.labelString: Use default settings\n"
"ecFlowview*@color_complete.labelString: Use default settings\n"
"ecFlowview*@color_halted.labelString: Use default settings\n"
"ecFlowview*@color_queued.labelString: Use default settings\n"
"ecFlowview*@color_shutdown.labelString: Use default settings\n"
"ecFlowview*@color_submitted.labelString: Use default settings\n"
"ecFlowview*@color_suspended.labelString: Use default settings\n"
"ecFlowview*@color_unknown.labelString: Use default settings\n"
"ecFlowview*@color_meter_low.labelString: Use default settings\n"
"ecFlowview*@color_threshold.labelString: Use default settings\n"
"ecFlowview*@color_event.labelString: Use default settings\n"
"ecFlowview*@direct_read.labelString: Use default settings\n"
"ecFlowview*@drift.labelString: Use default settings\n"
"ecFlowview*@late.labelString: Use default settings\n"
"ecFlowview*@maximum.labelString: Use default settings\n"
"ecFlowview*@jobfile_length.labelString: Use default settings\n"
"ecFlowview*@new_suites.labelString: Use default settings\n"
"ecFlowview*@normal_font_bold.labelString: Use default settings\n"
"ecFlowview*@normal_font_plain.labelString: Use default settings\n"
"ecFlowview*@poll.labelString: Use default settings\n"
"ecFlowview*@restarted.labelString: Use default settings\n"
"ecFlowview*@small_font_bold.labelString: Use default settings\n"
"ecFlowview*@small_font_plain.labelString: Use default settings\n"
"ecFlowview*@timeout.labelString: Use default settings\n"
"ecFlowview*timed_text_since_.labelString: 600\n"
"ecFlowview*timed_text_from_.labelString: 0\n"
"ecFlowview*Aborted.labelString: Aborted\n"
"ecFlowview*Active.labelString: Active\n"
"ecFlowview*Apply.labelString: Apply\n"
"ecFlowview*Backwards.labelString: Backwards\n"
"ecFlowview*Colours.labelString: Colors\n"
"ecFlowview*Complete.labelString: Complete\n"
"ecFlowview*Get server status:.labelString: Getting server status\n"
"ecFlowview*Hyper*highlightColor: Blue\n"
"ecFlowview*Hyper*highlightFont:  7x13\n"
"ecFlowview*Hyper*highlightOnEnter : false\n"
"ecFlowview*Hyper*highlightThickness : 0\n"
"ecFlowview*Hyper*navigationType : NONE\n"
"ecFlowview*Hyper*normalFont:     7x13\n"
"ecFlowview*Queued.labelString: Queued\n"
"ecFlowview*Regular expression.labelString: Regular expression\n"
"ecFlowview*SimpleTime.fontList: -*-*-*-*-*-*-7-*-*-*-*-*-*-*\n"
"ecFlowview*Submitted.labelString: Submitted\n"
"ecFlowview*Suspended.labelString: Suspended\n"
"ecFlowview*Unknown.labelString: Unknown\n"
"ecFlowview*Update.labelString: Update\n"
"ecFlowview*Use external editor.labelString: External editor...\n"
"ecFlowview*Use external viewer.labelString: External viewer...\n"
"ecFlowview*Waiting nodes.labelString: Waiting nodes\n"
"ecFlowview*XmToggleButton.fillOnSelect:       true\n"
"ecFlowview*XmToggleButton.selectColor:        Blue\n"
"ecFlowview*XmToggleButtonGadget.fillOnSelect:       true\n"
"ecFlowview*XmToggleButtonGadget.fontList:-*-helvetica-*-r-normal-*-12-*-*-*-*-*-*-*\n"
"ecFlowview*XmToggleButtonGadget.selectColor:        Green\n"
"ecFlowview*aborted.labelString: Aborted tasks\n"
"ecFlowview*alias_.labelString: Send as alias\n"
"ecFlowview*all_off.labelString: All off\n"
"ecFlowview*all_on.labelString: All on\n"
"ecFlowview*ask_shell.title: ask\n"
"ecFlowview*background: #e5e5e5e5e5e5\n"
"ecFlowview*bottomShadowColor: #7e7e7e7e7e7e\n"
"ecFlowview*button3.labelString: Help...\n"
"ecFlowview*button_close.labelString: Close\n"
"ecFlowview*button_find.labelString: Find...\n"
"ecFlowview*button_search.labelString: Search...\n"
"ecFlowview*close.labelString: Close\n"
"ecFlowview*close_on_apply_.labelString: Close on Apply/Submit\n"
"ecFlowview*collector_shell.title: Collector (ctrl-click1)\n"
  /* "ecFlowview*collector_shell.title: Collector\n" */
"ecFlowview*confirm_shell.title: Confirm\n"
"ecFlowview*current_node.labelString: Frozen\n"
"ecFlowview*delete_.labelString: Delete\n"
"ecFlowview*find_.labelString: Find\n"
"ecFlowview*depend_shell.title: Details\n"
"ecFlowview*dependencies_button_.labelString: Dependencies\n"
"ecFlowview*detached_.labelString: Detached\n"
"ecFlowview*direct_read.labelString: Read files from disk when possible.\n"
"ecFlowview*drift.labelString: Reduce call frequency when inactive\n"
"ecFlowview*error_shell.title: Error\n"
"ecFlowview*find_message.fontList:-*-helvetica-bold-*-normal-*-12-*-*-*-*-*-*-*\n"
"ecFlowview*find_message.foreground: red\n"
"ecFlowview*find_message.labelString: -\n"
"ecFlowview*find_shell.title: Find...\n"
"ecFlowview*fold_around_.labelString: Fold around\n"
"ecFlowview*XmList.fontList:	7x13=normal,7x13bold=bold\n"
"ecFlowview*XmText.fontList:	7x13\n"
"ecFlowview*fontList:-*-helvetica-normal-r-normal-*-12-*-*-*-*-*-*-*\n"
"ecFlowview*help_menu.labelString: Help\n"
"ecFlowview*hide_other_.labelString: Hide other suites\n"
"ecFlowview*indicatorSize:     12\n"
"ecFlowview*late.labelString: Late tasks\n"
"ecFlowview*mail_shell.title: Chat\n"
"ecFlowview*menu_fold_all.labelString: Fold all\n"
"ecFlowview*menu_show_current.labelString: Show selected node\n"
"ecFlowview*menu_unfold_all.accelerator: Ctrl<Key>U\n"
"ecFlowview*menu_unfold_all.labelString: Unfold all\n"
"ecFlowview*new_suites.labelString: Register to new suites\n"
"ecFlowview*new_window.labelString: New window...\n"
"ecFlowview*optionMenu1.labelString: Action:\n"
"ecFlowview*poll.labelString: Get server status regularly\n"
"ecFlowview*preprocess_.labelString: Pre-process\n"
"ecFlowview*restarted.labelString: Restarted tasks\n"
"ecFlowview*search_shell.title: Search...\n"
"ecFlowview*set_.labelString: Set\n"
"ecFlowview*timeline_label.fontList : 7x13=normal,7x13bold=bold\n"
"ecFlowview*toggle2.labelString: Limits\n"
"ecFlowview*toggle21.labelString: Suites\n"
"ecFlowview*toggle22.labelString: Families\n"
"ecFlowview*toggle23.labelString: Tasks\n"
"ecFlowview*toggle24.labelString: Aliases\n"
"ecFlowview*toggle25.labelString: Labels\n"
"ecFlowview*toggle26.labelString: Meters\n"
"ecFlowview*toggle27.labelString: Events\n"
"ecFlowview*toggle28.labelString: Repeats\n"
"ecFlowview*toggle29.labelString: Times\n"
"ecFlowview*toggle3.labelString: Full names\n"
"ecFlowview*toggle3.labelString: Limiters\n"
"ecFlowview*toggle30.labelString: Dates\n"
"ecFlowview*toggle31.labelString: Triggers\n"
"ecFlowview*toggle32.labelString: Variables\n"
"ecFlowview*toggle41.labelString: Time dependent\n"
"ecFlowview*toggle42.labelString: Late nodes\n"
"ecFlowview*toggle43.labelString: Zombie\n"
"ecFlowview*toggle44.labelString: Rerun tasks\n"
"ecFlowview*toggle45.labelString: Nodes with messages\n"
"ecFlowview*toggle66.labelString: Servers\n"
"ecFlowview*tools_*shadowThickness:1  \n"
"ecFlowview*topShadowColor: white\n"
"ecFlowview*triggered.labelString: Show triggered nodes\n"
"ecFlowview*triggers.labelString: Show triggering nodes\n"
"ecFlowview*vname.labelString: Variable name:\n"
"ecFlowview*vvalue.labelString: Variable value:\n"
"ecFlowview*warn.labelString: Don't forget to hit <return> when you enter a value in a text field.\n"
"ecFlowview*warn2.labelString: Don't forget to hit <return> when you enter a value in a text field.\n"
"ecFlowview*what_.labelString: What:\n"
"ecFlowview*where_.labelString: Where:\n"
"ecFlowview*why_label_.labelString: Use the Info window...\n"
"ecFlowview.smallFont:-*-helvetica-medium-r-normal-*-10-*-*-*-*-*-*-*\n"
"ecFlowview.normalFont:-*-helvetica-medium-r-normal-*-12-*-*-*-*-*-*-*\n"
"ecFlowview*timed_.labelString: State Time:\n"
"ecFlowview*timed_text_since_.labelString: Since:\n"
"ecFlowview*timed_text_from_.labelString: From:\n"
"ecFlowview*tools_.Info.accelerator: I\n"
"ecFlowview*tools_.Script.accelerator: S\n"
"ecFlowview*tools_.Manual.accelerator: M\n"
"ecFlowview*tools_.JobStatus.accelerator: J\n"
"ecFlowview*tools_.Output.accelerator: O\n"
"ecFlowview*tools_.Triggers.accelerator: T\n"
"ecFlowview*tools_.Why?.accelerator: W\n"
"ecFlowview*tools_.Variables.accelerator: V\n"
"ecFlowview*tools_.Messages.accelerator: M\n"
"ecFlowview*tools_.Edit.accelerator: E\n"
"ecFlowview*tools_.Status.accelerator: Return<Key>\n"
"ecFlowview*tree_.accelerator: C\n"
"ecFlowview*tree_.accelerator: Z\n"
"ecFlowview*tree_.baseTranslations: #augment \
C: collector::show(selection::current_node())\\n\
A: selection::notify_new_selection(0)\\n\
"

;

#else

static char* xresources = (char*) "\n"
"XCdp*File.accelerator: Ctrl<Key>f\n"
"XCdp*File.mnemonic: F\n"
"XCdp*Info.accelerator: Ctrl<Key>I\n"
"XCdp*Script.accelerator: Ctrl<Key>S\n"
"XCdp*Manual.accelerator: Ctrl<Key>M\n"
"XCdp*Jobstatus.accelerator: Ctrl<Key>j\n"
"XCdp*Job.accelerator: Ctrl<Key>J\n"
"XCdp*Triggers.accelerator: Ctrl<Key>T\n"
"XCdp*Why?.accelerator: Ctrl<Key>y\n"
"XCdp*Variables.accelerator: Ctrl<Key>V\n"
"XCdp*Messages.accelerator: Ctrl<Key>M\n"
"XCdp*Edit.accelerator: Ctrl<Key>E\n"
"XCdp*Search.accelerator: Ctrl<Key>c\n"
"XCdp*Status.accelerator: Space<Key>\n"
"XCdp*Login.accelerator: Ctrl<Key>L\n"
"XCdp*Login.acceleratorText: Ctrl+L\n"
"XCdp*Login.mnemonic: L\n"
"XCdp*Login.title: Login...\n"
"XCdp*Quit.accelerator: Ctrl<Key>Q\n"
"XCdp*Quit.acceleratorText: Ctrl+Q\n"
"XCdp*Quit.mnemonic: Q\n"
"XCdp*Edit.accelerator: Ctrl<Key>E\n"
"XCdp*Edit.mnemonic: E\n"
"XCdp*pref.accelerator: Ctrl<Key>e\n"
"XCdp*pref.mnemonic: e\n"
"XCdp*pref.title: User preferences...\n"

"XCdp*pref_shell.title: Preferences\n"

"XCdp*Show.accelerator: Ctrl<Key>S\n"
"XCdp*Show.mnemonic: S\n"
"XCdp*Servers.accelerator: Ctrl<Key>v\n"
"XCdp*Servers.mnemonic: v\n"
"XCdp*Windows.accelerator: Ctrl<Key>W\n"
"XCdp*Windows.mnemonic: W\n"
"XCdp*Help.accelerator: Ctrl<Key>H\n"
"XCdp*Help.mnemonic: H\n"

"XCdp*Version.accelerator: Ctrl<Key>V\n"
"XCdp*Version.mnemonic: V\n"

"XCdp*file_menu.labelString: File\n"
"XCdp*file_menu.mnemonic: F\n"
"XCdp*Option.mnemonic: O\n"
"XCdp*Print*.mnemonic: P\n"
"XCdp*Save*.mnemonic: S\n"
"XCdp*close.mnemonic: C\n"
"XCdp*help_menu.mnemonic: H\n"

"XCdp*snapshot.accelerator: Ctrl<Key>t\n"
"XCdp*snapshot.labelString: Snapshot\n"

"XCdp*file_menu.title: File\n"
"XCdp*file_menu.accelerator: Ctrl<Key>F\n"

"XCdp*options_menu.labelString: Options\n"
"XCdp*options_menu.mnemonic: O\n"

"XCdp.*SimpleBase.baseTranslations: #augment \
 Shift<Btn5Down>: increment(-1)  \\n\
 Shift<Btn4Down>: increment(1)   \\n\
      <Btn5Down>: increment(-10) \\n\
      <Btn4Down>: increment(10)  \n"

"XCdp.*Hyper.baseTranslations: #augment \
 Shift<Btn5Down>: increment(-1)  \\n\
 Shift<Btn4Down>: increment(1)   \\n\
      <Btn5Down>: increment(-10) \\n\
      <Btn4Down>: increment(10)  \n"

"XCdp.*XmList.baseTranslations: #augment \
 Shift<Btn5Down>: ListNextPage() \\n\
 Shift<Btn4Down>: ListPrevPage() \\n\
      <Btn5Down>: ListNextItem() \\n\
      <Btn4Down>: ListPrevItem() \n"

"XCdp.*XmScrollBar.baseTranslations: #augment \
 Shift<Btn5Down>: IncrementDownOrRight(0) IncrementDownOrRight(1) \\n\
 Shift<Btn4Down>: IncrementUpOrLeft(0) IncrementUpOrLeft(1) \\n\
      <Btn5Down>: PageDownOrRight(0) PageDownOrRight(1) \\n\
      <Btn4Down>: PageUpOrLeft(0) PageUpOrLeft(1) \n"

"XCdp.*XmText.baseTranslations: #augment    \
      <Key>osfUp:      scroll-one-line-up()\\n\
      <Key>osfDown:    scroll-one-line-down()\\n\
      <KeyUp>Prior:    previous-page()\\n\
      <KeyUp>Next:     next-page()\\n\
      <Key>Up:         scroll-one-line-up()   \\n\
      <Key>Down:       scroll-one-line-down() \\n\
      <KeyUp>KP_Prior: previous-page()\\n\
      <KeyUp>KP_Next:  next-page()\\n\
      <KeyUp>KP_Up:    scroll-one-line-up()   \\n\
      <KeyUp>KP_Down:  scroll-one-line-down() \n\
 Shift<Btn5Down>: previous-page()   \\n\
 Shift<Btn4Down>: next-page() \\n\
      <Btn5Down>: scroll-one-line-up()   \\n\
      <Btn4Down>: scroll-one-line-down() \n"

"XCdp.*DrawingAreaInput.baseTranslations: #augment    \
      <Key>osfUp:      scroll-one-line-up()\\n\
      <Key>osfDown:    scroll-one-line-down()\\n\
      <KeyUp>Prior:    previous-page()\\n\
      <KeyUp>Next:     next-page()\\n\
      <Key>Up:         scroll-one-line-up()   \\n\
      <Key>Down:       scroll-one-line-down() \\n\
      <KeyUp>KP_Prior: previous-page()\\n\
      <KeyUp>KP_Next:  next-page()\\n\
      <KeyUp>KP_Up:    scroll-one-line-up()   \\n\
      <KeyUp>KP_Down:  scroll-one-line-down() \n\
 Shift<Btn5Down>: previous-page()   \\n\
 Shift<Btn4Down>: next-page() \\n\
      <Btn5Down>: scroll-one-line-up()   \\n\
      <Btn4Down>: scroll-one-line-down() \n"

"*XmSpinBox.accelerators: #augment \
      <Btn5Down>: SpinBPrior()\\n\
      <Btn5Up>:SpinBDisarm()\\n\
      <Btn4Down>: SpinBNext()\\n\
      <Btn4Up>: SpinBDisarm()\\n\
      <Key>osfUp:SpinBNext()\\n\
      <Key>osfDown: SpinBPrior()\\n\
      <KeyUp>osfUp: SpinBDisarm()\\n\
      <KeyUp>osfDown: SpinBDisarm()\\n\
      <Key>osfLeft:  SpinBLeft()\\n\
      <Key>osfRight: SpinBRight()\\n\
      <KeyUp>osfLeft:  SpinBDisarm()\\n\
      <KeyUp>osfRight: SpinBDisarm()\\n\
      <Key>osfBeginLine: SpinBFirst()\\n\
      <Key>osfEndLine: SpinBLast()\n"

"XCdp*@zombied.labelString: Use default settings\n"
"XCdp*@aliases.labelString: Use default settings\n"
"XCdp*zombied.labelString: Zombies\n"
"XCdp*aliases.labelString: Aborted or restarted aliases\n"

"XCdp*File.labelString: File\n"
"XCdp*.XmText.background: OldLace\n"
"XCdp*.XmTextField.background: OldLace\n"
"XCdp*.scrollBarDisplayPolicy: STATIC\n"
"XCdp*@aborted.labelString: Use default settings\n"
"XCdp*@color_aborted.labelString: Use default settings\n"
"XCdp*@color_active.labelString: Use default settings\n"
"XCdp*@color_complete.labelString: Use default settings\n"
"XCdp*@color_halted.labelString: Use default settings\n"
"XCdp*@color_queued.labelString: Use default settings\n"
"XCdp*@color_shutdown.labelString: Use default settings\n"
"XCdp*@color_submitted.labelString: Use default settings\n"
"XCdp*@color_suspended.labelString: Use default settings\n"
"XCdp*@color_unknown.labelString: Use default settings\n"
"XCdp*@color_meter_low.labelString: Use default settings\n"
"XCdp*@color_threshold.labelString: Use default settings\n"
"XCdp*@color_event.labelString: Use default settings\n"
"XCdp*@direct_read.labelString: Use default settings\n"
"XCdp*@drift.labelString: Use default settings\n"
"XCdp*@late.labelString: Use default settings\n"
"XCdp*@maximum.labelString: Use default settings\n"
"XCdp*@jobfile_length.labelString: Use default settings\n"
"XCdp*@new_suites.labelString: Use default settings\n"
"XCdp*@normal_font_bold.labelString: Use default settings\n"
"XCdp*@normal_font_plain.labelString: Use default settings\n"
"XCdp*@poll.labelString: Use default settings\n"
"XCdp*@restarted.labelString: Use default settings\n"
"XCdp*@small_font_bold.labelString: Use default settings\n"
"XCdp*@small_font_plain.labelString: Use default settings\n"
"XCdp*@timeout.labelString: Use default settings\n"
"XCdp*Aborted.labelString: Aborted\n"
"XCdp*Active.labelString: Active\n"
"XCdp*Apply.labelString: Apply\n"
"XCdp*Backwards.labelString: Backwards\n"
"XCdp*Colours.labelString: Colors\n"
"XCdp*Complete.labelString: Complete\n"
"XCdp*Get server status:.labelString: Getting server status\n"
"XCdp*Hyper*highlightColor: Blue\n"
"XCdp*Hyper*highlightFont:  7x13\n"
"XCdp*Hyper*highlightOnEnter : false\n"
"XCdp*Hyper*highlightThickness : 0\n"
"XCdp*Hyper*navigationType : NONE\n"
"XCdp*Hyper*normalFont:     7x13\n"
"XCdp*Queued.labelString: Queued\n"
"XCdp*Regular expression.labelString: Regular expression\n"
"XCdp*SimpleTime.fontList: -*-*-*-*-*-*-7-*-*-*-*-*-*-*\n"
"XCdp*Submitted.labelString: Submitted\n"
"XCdp*Suspended.labelString: Suspended\n"
"XCdp*Unknown.labelString: Unknown\n"
"XCdp*Update.labelString: Update\n"
"XCdp*Use external editor.labelString: External editor...\n"
"XCdp*Use external viewer.labelString: External viewer...\n"
"XCdp*Waiting nodes.labelString: Waiting nodes\n"
"XCdp*XmToggleButton.fillOnSelect:       true\n"
"XCdp*XmToggleButton.selectColor:        Blue\n"
"XCdp*XmToggleButtonGadget.fillOnSelect:       true\n"
"XCdp*XmToggleButtonGadget.fontList:-*-helvetica-normal-r-normal-*-12-*-*-*-*-*-*-*\n"
"XCdp*XmToggleButtonGadget.selectColor:        Green\n"
"XCdp*aborted.labelString: Aborted tasks\n"
"XCdp*alias_.labelString: Send as alias\n"
"XCdp*all_off.labelString: All off\n"
"XCdp*all_on.labelString: All on\n"
"XCdp*ask_shell.title: ask\n"
"XCdp*background: #e5e5e5e5e5e5\n"
"XCdp*bottomShadowColor: #7e7e7e7e7e7e\n"
"XCdp*button3.labelString: Help...\n"
"XCdp*button_close.labelString: Close\n"
"XCdp*button_find.labelString: Find...\n"
"XCdp*button_search.labelString: Search...\n"
"XCdp*close.labelString: Close\n"
"XCdp*close_on_apply_.labelString: Close on Apply/Submit\n"
"XCdp*collector_shell.title: Collector (ctrl-click1)\n"
  /* "XCdp*collector_shell.title: Collector\n" */
"XCdp*confirm_shell.title: Confirm\n"
"XCdp*current_node.labelString: Frozen\n"
"XCdp*delete_.labelString: Delete\n"
"XCdp*find_.labelString: Find\n"
"XCdp*depend_shell.title: Details\n"
"XCdp*dependencies_button_.labelString: Dependencies\n"
"XCdp*detached_.labelString: Detached\n"
"XCdp*direct_read.labelString: Read output and other files from disk when possible.\n"
"XCdp*drift.labelString: Reduce call frequency when inactive\n"
"XCdp*error_shell.title: Error\n"
"XCdp*find_message.fontList:-*-helvetica-bold-*-normal-*-12-*-*-*-*-*-*-*\n"
"XCdp*find_message.foreground: red\n"
"XCdp*find_message.labelString: -\n"
"XCdp*find_shell.title: Find...\n"
"XCdp*fold_around_.labelString: Fold around\n"
"XCdp*XmList.fontList:	7x13=normal,7x13bold=bold\n"
"XCdp*XmText.fontList:	7x13\n"
"XCdp*fontList:-*-helvetica-normal-r-normal-*-12-*-*-*-*-*-*-*\n"
"XCdp*help_menu.labelString: Help\n"
"XCdp*hide_other_.labelString: Hide other suites\n"
"XCdp*indicatorSize:     12\n"
"XCdp*late.labelString: Late tasks\n"
"XCdp*mail_shell.title: Chat\n"
"XCdp*menu_fold_all.labelString: Fold all\n"
"XCdp*menu_show_current.labelString: Show selected node\n"
"XCdp*menu_unfold_all.accelerator: Ctrl<Key>U\n"
"XCdp*menu_unfold_all.labelString: Unfold all\n"
"XCdp*new_suites.labelString: Register to new suites\n"
"XCdp*new_window.labelString: New window...\n"
"XCdp*optionMenu1.labelString: Action:\n"
"XCdp*poll.labelString: Get server status regularly\n"
"XCdp*preprocess_.labelString: Pre-process\n"
"XCdp*restarted.labelString: Restarted tasks\n"
"XCdp*search_shell.title: Search...\n"
"XCdp*set_.labelString: Set\n"
"XCdp*timeline_label.fontList : 7x13=normal,7x13bold=bold\n"
"XCdp*toggle2.labelString: Limits\n"
"XCdp*toggle21.labelString: Suites\n"
"XCdp*toggle22.labelString: Families\n"
"XCdp*toggle23.labelString: Tasks\n"
"XCdp*toggle24.labelString: Aliases\n"
"XCdp*toggle25.labelString: Labels\n"
"XCdp*toggle26.labelString: Meters\n"
"XCdp*toggle27.labelString: Events\n"
"XCdp*toggle28.labelString: Repeats\n"
"XCdp*toggle29.labelString: Times\n"
"XCdp*toggle3.labelString: Full names\n"
"XCdp*toggle3.labelString: Limiters\n"
"XCdp*toggle30.labelString: Dates\n"
"XCdp*toggle31.labelString: Triggers\n"
"XCdp*toggle32.labelString: Variables\n"
"XCdp*toggle41.labelString: Time dependent\n"
"XCdp*toggle42.labelString: Late nodes\n"
"XCdp*toggle43.labelString: Zombie\n"
"XCdp*toggle44.labelString: Rerun tasks\n"
"XCdp*toggle45.labelString: Nodes with messages\n"
"XCdp*toggle66.labelString: Servers\n"
"XCdp*tools_*shadowThickness:1  \n"
"XCdp*topShadowColor: white\n"
"XCdp*triggered.labelString: Show triggered nodes\n"
"XCdp*triggers.labelString: Show triggering nodes\n"
"XCdp*vname.labelString: Variable name:\n"
"XCdp*vvalue.labelString: Variable value:\n"
"XCdp*warn.labelString: Don't forget to hit <return> when you enter a value in a text field.\n"
"XCdp*warn2.labelString: Don't forget to hit <return> when you enter a value in a text field.\n"
"XCdp*what_.labelString: What:\n"
"XCdp*where_.labelString: Where:\n"
"XCdp*why_label_.labelString: Use the Info window...\n"
"XCdp.smallFont:-*-helvetica-medium-r-normal-*-11-*-*-*-*-*-*-*\n"
"XCdp*timed_.labelString: State Time:\n"
"XCdp*timed_text_since_.labelString: Since:\n"
"XCdp*timed_text_from_.labelString: From:\n"
"XCdp*tools_.Info.accelerator: I\n"
"XCdp*tools_.Script.accelerator: S\n"
"XCdp*tools_.Manual.accelerator: M\n"
"XCdp*tools_.JobStatus.accelerator: J\n"
"XCdp*tools_.Output.accelerator: O\n"
"XCdp*tools_.Triggers.accelerator: T\n"
"XCdp*tools_.Why?.accelerator: W\n"
"XCdp*tools_.Variables.accelerator: V\n"
"XCdp*tools_.Messages.accelerator: M\n"
"XCdp*tools_.Edit.accelerator: E\n"
"XCdp*tools_.Status.accelerator: Return<Key>\n"
"XCdp*tree_.accelerator: C\n"
"XCdp*tree_.accelerator: Z\n"
"XCdp*tree_.baseTranslations: #augment \
C: collector::show(selection::current_node())\\n\
A: selection::notify_new_selection(0)\\n\
"

;

/* 
   xrdb -merge ~/.Xdefaults 
*/
#endif
