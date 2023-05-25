//============================================================================
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "VFile.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <unistd.h>

#include <boost/lexical_cast.hpp>
#include <sys/stat.h>
#include <sys/types.h>

#include "DirectoryHandler.hpp"
#include "UiLog.hpp"

#define UI_VFILE_DEBUG_

const size_t VFile::maxDataSize_ = 1024 * 1024 * 10;

VFile::VFile(const std::string& name, const std::string& str, bool deleteFile)
    : path_(name),
      deleteFile_(deleteFile),
      storageMode_(DiskStorage),
      data_(nullptr),
      dataSize_(0),
      fp_(nullptr),
      fetchMode_(NoFetchMode),
      transferDuration_(0),
      truncatedTo_(0),
      cached_(false) {
    std::ofstream f(path_.c_str());
    if (f.is_open()) {
        f << str;
        f.close();
    }
}

VFile::VFile(const std::string& name, bool deleteFile)
    : path_(name),
      deleteFile_(deleteFile),
      storageMode_(DiskStorage),
      data_(nullptr),
      dataSize_(0),
      fp_(nullptr),
      fetchMode_(NoFetchMode),
      transferDuration_(0),
      truncatedTo_(0),
      cached_(false) {
}

VFile::VFile(bool deleteFile) : path_(""), deleteFile_(deleteFile) {
}

VFile::~VFile() {
    close();

    UiLog().dbg() << UI_FN_INFO;
    print();

    if (data_) {
        delete[] data_;
        UiLog().dbg() << " memory released";
    }

    // TODO: add further/better checks
    if (deleteFile_ && exists() && !path_.empty() && path_ != "/" && path_.size() > 4) {
        unlink(path_.c_str());
        UiLog().dbg() << " file deleted from disk";
    }
    else if (!path_.empty() && exists()) {
        UiLog().dbg() << " file was kept on disk";
    }
}

bool VFile::exists() const {
    if (path_.empty())
        return false;
    return (access(path_.c_str(), R_OK) == 0);
}

VFile_ptr VFile::create(const std::string& path, const std::string& str, bool deleteFile) {
    return VFile_ptr(new VFile(path, str, deleteFile));
}

VFile_ptr VFile::create(const std::string& path, bool deleteFile) {
    return VFile_ptr(new VFile(path, deleteFile));
}

VFile_ptr VFile::create(bool deleteFile) {
    return VFile_ptr(new VFile(deleteFile));
}

VFile_ptr VFile::createTmpFile(bool deleteFile) {
    std::string tmpFile = DirectoryHandler::tmpFileName();
    std::ofstream f(tmpFile.c_str());
    if (f.is_open()) {
        f.close();
    }

    return VFile_ptr(new VFile(tmpFile, deleteFile));
}

void VFile::setStorageMode(StorageMode mode) {
    if (storageMode_ == mode)
        return;

    storageMode_ = mode;

    if (storageMode_ == DiskStorage) {
        if (dataSize_ > 0) {
            if (path_.empty())
                path_ = DirectoryHandler::tmpFileName();

            fp_ = fopen(path_.c_str(), "w");
            if (fwrite(data_, 1, dataSize_, fp_) != dataSize_) {}
            fclose(fp_);
            fp_ = nullptr;
            delete[] data_;
            data_     = nullptr;
            dataSize_ = 0;
        }
    }
}

bool VFile::write(const std::string& buf, std::string& err) {
    return write(buf.c_str(), buf.size(), err);
}

bool VFile::write(const char* buf, size_t len, std::string& /*err*/) {
    if (len == 0) {
        return true;
    }

    // printf("total:%d \n len: %d \n",dataSize_,len);

    // Keep data in memory
    if (storageMode_ == MemoryStorage) {
        if (!data_) {
            data_ = new char[maxDataSize_ + 1];
        }

        if (dataSize_ + len < maxDataSize_) {
            memcpy(data_ + dataSize_, buf, len);
            dataSize_ += len;
            data_[dataSize_] = '\0'; // terminate the string
            // UiLog().dbg() << "VFile::write dataSize=" << dataSize_;
            return true;
        }
        else {
            setStorageMode(DiskStorage);
        }
    }

    // Write data to disk
    if (storageMode_ == DiskStorage) {
        if (!fp_) {
            if (path_.empty())
                path_ = DirectoryHandler::tmpFileName();

            fp_ = fopen(path_.c_str(), "a");
        }

        if (fwrite(buf, 1, len, fp_) != len) {
            // char buf_loc[2048];
            // sprintf(buf_loc,"Write error on %s",out->path().c_str());
            // gui::syserr(buf);
            fclose(fp_);
            return false;
        }
        fflush(fp_);
    }

    return true;
}

void VFile::close() {
    if (fp_) {
        fclose(fp_);
        fp_ = nullptr;
    }
    if (data_) {
        data_[dataSize_] = '\0';
    }
}

bool VFile::appendContentsTo(FILE* fpTarget) const {
    if (storageMode_ == DiskStorage) {
        FILE* fp = fopen(path_.c_str(), "r");
        if (fp == nullptr) {
            return false;
        }
        char buf[8 * 1024];
        size_t n = 0;
        while ((n = fread(buf, 1, sizeof(buf), fp)) > 0) {
            if (fwrite(buf, 1, n, fpTarget) != n) {
                fclose(fp);
                return false;
            }
        }
        fclose(fp);
    }
    else if (storageMode_ == MemoryStorage) {
        if (data_ && dataSize_ > 0) {
            if (fwrite(data_, 1, dataSize_, fpTarget) != dataSize_) {
                return false;
            }
        }
    }

    return true;
}

// Append the contents of other to the current object
bool VFile::append(VFile_ptr other) {
    if (!other || !other->hasDeltaContents() || other->sizeInBytes() == 0) {
#ifdef UI_VFILE_DEBUG_
        UiLog().dbg() << UI_FN_INFO << " empty or has no delta";
#endif
        return false;
    }

    // TODO: assert/exit if this happens?
    if (fetchMode_ != other->fetchMode_) {
        return false;
    }

    if (fetchMode_ == LogServerFetchMode || fetchMode_ == TransferFetchMode) {
        // adjust the storatge mode according to the expected new size
        if (other->storageMode() == DiskStorage) {
            setStorageMode(DiskStorage);
        }
        if (storageMode_ == MemoryStorage && other->storageMode() == MemoryStorage) {
            if (dataSize_ + other->dataSize_ > maxDataSize_) {
                setStorageMode(DiskStorage);
            }
        }

        fetchDate_        = other->fetchDate_;
        transferDuration_ = other->transferDuration_;
        sourceModTime_    = other->sourceModTime_;
        sourceCheckSum_   = other->sourceCheckSum_;

        if (storageMode_ == DiskStorage) {
            if (FILE* fp = fopen(path_.c_str(), "a")) {
                other->appendContentsTo(fp);
                fclose(fp);
            }
            else {
                UiLog().err() << UI_FN_INFO << "could not open file=" << path_ << " in append mode";
            }
        }
        else if (storageMode_ == MemoryStorage) {
            assert(other->storageMode() == MemoryStorage);
            assert(dataSize_ + other->dataSize_ <= maxDataSize_);
            if (other->dataSize() > 0) {
                if (!data_) {
                    data_ = new char[maxDataSize_ + 1];
                }
                memcpy(data_ + dataSize_, other->data_, other->dataSize_);
                dataSize_ += other->dataSize_;
                data_[dataSize_] = '\0'; // terminate the string
                //                UiLog().dbg() << "VFile::append prev=" << prev << " current=" << dataSize_ <<
                //                                 " other->data=|" << other->data_ << "|";
            }
            else {
                return false;
            }
        }
        return true;
    }

    return false;
}

bool VFile::isEmpty() const {
    if (storageMode_ == VFile::MemoryStorage)
        return dataSize_ == 0;

    if (exists()) {
        struct stat info;
        return (::stat(path_.c_str(), &info) != 0 || info.st_size == 0);
    }
    return false;
}

size_t VFile::fileSize() const {
    if (storageMode_ == VFile::DiskStorage) {
        if (exists()) {
            struct stat info;
            if (::stat(path_.c_str(), &info) == 0)
                return info.st_size;
        }
    }
    return 0;
}

size_t VFile::sizeInBytes() const {
    return (storageMode_ == VFile::MemoryStorage) ? dataSize_ : fileSize();
}

int VFile::numberOfLines() const {
    if (storageMode_ != VFile::DiskStorage || isEmpty())
        return -1;

    int num  = 0;
    int c    = 0;
    FILE* fp = fopen(path_.c_str(), "r");
    if (!fp)
        return -1;

    for (c = getc(fp); c != EOF; c = getc(fp))
        if (c == '\n')
            num++;

    fclose(fp);
    return num;
}

void VFile::print() {
    std::string str = " VFile storage=" + std::to_string(storageMode_) + " ";
    if (storageMode_ == MemoryStorage) {
        str += "memory size:" + boost::lexical_cast<std::string>(dataSize_);
    }
    else {
        str += "disk path: " + path_;
    }

    UiLog().dbg() << str;
}
