//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef VREPLY_HPP_
#define VREPLY_HPP_

#include <string>
#include <vector>

#include "Zombie.hpp"
#include "VFile.hpp"
#include "VDir.hpp"

class VReply
{
public:
	enum Status {NoStatus,TaskDone,TaskFailed,TaskCancelled};
	enum FileReadMode {NoReadMode,LocalReadMode,ServerReadMode,LogServerReadMode};
    VReply(void* sender=NULL) : sender_(sender), status_(NoStatus), readMode_(NoReadMode),readTruncatedTo_(-1) {}
	~VReply() {}

	void reset();

	void* sender() const {return sender_;}
	void setSender(void* s) {sender_=s;}

    std::string errorText(const std::string& sep="") const;
    const std::vector<std::string>& errorTextVec() const {return errorText_;}
    std::string warningText(const std::string& sep="") const;
    std::string infoText(const std::string& sep="") const;
	const std::string& text() const {return text_;}
	const std::vector<std::string>& textVec() const {return textVec_;}
	Status status() const {return status_;}
    const std::string& fileName() const {return fileName_;}
	FileReadMode fileReadMode() const {return readMode_;}
    const std::string& fileReadMethod() {return readMethod_;}
	VFile_ptr tmpFile() const {return tmpFile_;}
    VDir_ptr directory() const {return dir_;}
    std::vector<VDir_ptr> directories() const {return dirs_;}
	const std::vector<Zombie>& zombies() const {return zombies_;}
	void setReadTruncatedTo(int ival) {readTruncatedTo_=ival;}
    const std::vector<std::string>& log() const {return log_;}

	bool textFromFile(const std::string&);
	void text(const std::vector<std::string>& msg);
	void setTextVec(const std::vector<std::string>& msg) {textVec_=msg;;}
	void text(const std::string& s) {text_=s;}
    void setErrorText(const std::string& s);
    void appendErrorText(const std::string& s);
    void setWarningText(const std::string& s);
    void appendWarningText(const std::string& s);
    void setInfoText(const std::string& s);
    void appendInfoText(const std::string& s);
	void fileName(const std::string& s) {fileName_=s;}
	void fileReadMode(FileReadMode m) {readMode_=m;}
	void fileReadMethod(const std::string& m) {readMethod_=m;}
	void tmpFile(VFile_ptr f) {tmpFile_=f;}
    void setDirectory(VDir_ptr d) {dir_=d;}
    void setDirectories(const std::vector<VDir_ptr>& d) {dirs_=d;}
	void zombies(const std::vector<Zombie>& z) { zombies_=z;}
	int readTruncatedTo() const {return readTruncatedTo_;}
    void addLog(const std::string& s) {log_.push_back(s);}
    void setLog(const std::vector<std::string>& s) {log_=s;}
    void clearLog() {log_.clear();}
    void setText(const std::string& txt) {text_=txt;}

    bool hasWarning() const {return (!warningText_.empty() && !warningText_[0].empty());}
    bool hasInfo() const {return (!infoText_.empty() && !infoText_[0].empty());}

	void prependText(const std::string&);
	void appendText(const std::string&);

	void status(Status s) {status_=s;}

protected:
	void* sender_;
	Status status_;
    std::vector<std::string> errorText_;
    std::vector<std::string> warningText_;
    std::vector<std::string> infoText_;
	std::string text_;
	std::vector<std::string> textVec_;
	std::string fileName_;
	FileReadMode readMode_;
	std::string  readMethod_;
	int readTruncatedTo_;
    std::vector<std::string> log_;
	VFile_ptr  tmpFile_;
    VDir_ptr dir_;
    std::vector<VDir_ptr> dirs_;
	std::vector<Zombie> zombies_;
};

#endif
