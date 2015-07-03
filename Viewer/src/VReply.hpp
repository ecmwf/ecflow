//============================================================================
// Copyright 2014 ECMWF.
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

class VReply
{
public:
	enum Status {NoStatus,TaskDone,TaskFailed,TaskCancelled};
	enum FileReadMode {NoReadMode,LocalReadMode,ServerReadMode,LogServerReadMode};
	VReply() : status_(NoStatus), readMode_(NoReadMode) {};
	~VReply() {}

	void reset();

	const std::string& errorText() const {return errorText_;}
	const std::string& text() const {return text_;}
	Status status() const {return status_;}
	const std::string fileName() const {return fileName_;}
	FileReadMode fileReadMode() const {return readMode_;}
	const std::string fileReadMethod() {return readMethod_;}
	VFile_ptr tmpFile() const {return tmpFile_;}
	const std::vector<Zombie>& zombies() const {return zombies_;}

	bool textFromFile(const std::string&);
	void text(const std::vector<std::string>& msg);
	void text(const std::string& s) {text_=s;}
	void errorText(const std::string& s) {errorText_=s;}
	void fileName(const std::string& s) {fileName_=s;}
	void fileReadMode(FileReadMode m) {readMode_=m;}
	void fileReadMethod(const std::string& m) {readMethod_=m;}
	void tmpFile(VFile_ptr f) {tmpFile_=f;}
	void zombies(const std::vector<Zombie>& z) { zombies_=z;}

	void prependText(const std::string&);
	void appendText(const std::string&);

	void status(Status s) {status_=s;}

protected:
	Status status_;
	std::string errorText_;
	std::string text_;
	std::string fileName_;
	FileReadMode readMode_;
	std::string  readMethod_;
	VFile_ptr  tmpFile_;
	std::vector<Zombie> zombies_;
};

#endif
