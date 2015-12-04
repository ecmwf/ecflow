//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "OutputClient.hpp"

#include <QTimer>

#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "setjmp.h"

#ifdef AIX
#include <memory.h>
#endif

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#define FAIL(a) do { perror(a); exit(1); } while(0)

OutputClient::OutputClient(const std::string& host,const std::string& portStr,QObject* parent) :
	QObject(parent),
	soc_(NULL),
	host_(host),
	portStr_(portStr),
	port_(19999),
	timeout_(3000),
	total_(0),
	lastProgress_(0),
	progressChunk_(1024*1024),
	progressUnits_("MB")
{
	if(!portStr_.empty())
		port_=atoi(portStr.c_str());

	soc_=new QTcpSocket(0);
	connect(soc_,SIGNAL(readyRead()),
		  this, SLOT(slotRead()));

	connect(soc_,SIGNAL(error(QAbstractSocket::SocketError)),
			this,SLOT(slotError(QAbstractSocket::SocketError)));

	connect(soc_,SIGNAL(connected()),
			  this, SLOT(slotConnected()));

	if(char* timeoutStr = getenv ("ECFLOWVIEW_LOGTIMEOUT"))
		timeout_ = atoi(timeoutStr)*1000;
}

void OutputClient::connectToHost(std::string host,int port)
{
	stopper_.start();
	soc_->abort();
	soc_->connectToHost(QString::fromStdString(host),port);

	//We cannot change the temout through the qt api so we need this hack.
	QTimer::singleShot(timeout_, this, SLOT(slotCheckTimeout()));
}

void OutputClient::slotCheckTimeout()
{
	if(soc_->state() == QAbstractSocket::HostLookupState ||
	   soc_->state() == QAbstractSocket::ConnectingState)
	{
		soc_->abort();
		if(out_)
			out_.reset();

		Q_EMIT error("Timeout error");
	}
}

void OutputClient::slotConnected()
{
	qDebug() << "connected to " << soc_->peerName();

	soc_->write("get ",4);
	soc_->write(remoteFile_.c_str(),remoteFile_.size());
	soc_->write("\n",1);
}

void OutputClient::slotError(QAbstractSocket::SocketError err)
{
	qDebug() << "error" <<  soc_->errorString();

	switch(err)
	{
	case QAbstractSocket::RemoteHostClosedError:
		qDebug() << "remote host closed";

		if(out_)
		{
			out_->setTransferDuration(stopper_.elapsed());
			out_->setFetchDate(QDateTime::currentDateTime());
			out_->close();
		}

		Q_EMIT finished();

		if(out_)
			out_.reset();

		break;
	default:
		soc_->abort();
		if(out_)
		{
			out_->close();
			out_.reset();
		}
		Q_EMIT error(soc_->errorString());
		break;
	}
}

void OutputClient::getFile(const std::string& name)
{
	connectToHost(host_,port_);

	remoteFile_=name;
	out_.reset();
	out_=VFile_ptr(VFile::create(false));
	total_=0;
}

void OutputClient::slotRead()
{
	const qint64 size = 64*1024;
	char buf[size];
	quint64 len = 0;

	//if(soc_->bytesAvailable() < (int)sizeof(quint16))
	//	return;

	while((len = soc_->read(buf,size)) > 0)
	{
		std::string err;
		if(!out_->write(buf,len,err))
		{
			soc_->abort();
			Q_EMIT error("write failed");
		}
		total_ += len;

		if(total_/progressChunk_ > lastProgress_)
		{
			lastProgress_=total_/progressChunk_;
			Q_EMIT progress(QString::number(lastProgress_) + " " + progressUnits_ + " read");

		}
	}

	qDebug() << "total" << total_;

	//in.setVersion(QDataStream::Qt_4_0);



	/*
	if(blockSize == 0) {
	  if (tcpSocket->bytesAvailable() < (int)sizeof(quint16))
	            return;

	        in >> blockSize;
	    }



	tcpSoc_->writeData("get ",4);
	tcpSoc_->write(name.c_str(),name.size());
	tcpSoc_->write("\n",1);

	*/


}

VFile_ptr OutputClient::resultFile() const
{
	return out_;
}

/*

void LogClient::connect(std::string host,int port)
{
	//typedef unsigned long addr_type;
	typedef in_addr_t addr_type;

	//ddr_type none = (addr_type)-1;
	addr_type addr;

	struct sockaddr_in s_in;
	struct hostent *him;

	soc_ = socket(AF_INET, SOCK_STREAM, 0);
	if(soc_ < 0)
	{
		//gui::syserr("Cannot create socket");
		return;
	}

	bzero(&s_in,sizeof(s_in));

	s_in.sin_port = htons(port);
	s_in.sin_family = AF_INET;
	addr = inet_addr(host.c_str());
	s_in.sin_addr.s_addr = addr;

#ifdef SVR4
	if(addr == (in_addr_t) 0xffffffff)
#else
	if(addr == INADDR_NONE)
#endif
	{
		if ((him=gethostbyname(host.c_str()))==NULL)
		{
			//gui::error("Unknown Host %s",host.c_str());
			return;
		}
		s_in.sin_family = him->h_addrtype;
		bcopy(him->h_addr_list[0],&s_in.sin_addr,him->h_length);
	}

	char* timeout = getenv ("ECFLOWVIEW_LOGTIMEOUT");
	int time_out = timeout ? atoi(timeout) : 3;

	struct sigaction sa = { { 0, },  };
	struct sigaction old;
	sa.sa_handler = catch_alarm;
	sigemptyset(&sa.sa_mask);

	if(sigaction(SIGALRM, &sa, &old))
		perror("sigaction");

	::alarm(time_out);
	perror("alarm");

	if(setjmp(env) == 0)
	{
		printf("connect %s\n",host.c_str());
	    if(::connect(soc_,(struct sockaddr*)&s_in,sizeof(s_in)) < 0)
		{
			perror("connect");
			close(soc_);
			soc_ = -1;
		}
	}
	else
	{
		printf("cleanup up\n");
		close(soc_);
		soc_ = -1;
	}
	::alarm(0);
	sigaction(SIGALRM, &old, &sa);
}
*/
OutputClient::~OutputClient()
{
	//close(soc_);
}
/*
VFile_ptr LogClient::getFile(std::string name)
{
	VFile_ptr empty;

    if(soc_ < 0)
    	return empty;

	write(soc_,"get ",4);
	write(soc_,name.c_str(),name.size());
	write(soc_,"\n",1);

	const int size = 64*1024;
	char buf[size];
	unsigned int len = 0;
	int total = 0;

	VFile_ptr out(VFile::create(false));
	FILE *f = fopen(out->path().c_str(),"w");

	printf("outFile: %s\n",out->path().c_str());


	if(!f)
	{
	  char buf_loc[2048];
	  sprintf(buf_loc,"Cannot create %s",out->path().c_str());
	  //gui::syserr(buf);
	  return empty;
	}

	while( (len = read(soc_,buf,size)) > 0)
	{
	  if(fwrite(buf,1,len,f) != len)
	    {
	      char buf_loc[2048];
	      sprintf(buf_loc,"Write error on %s",out->path().c_str());
	      //gui::syserr(buf);
	      fclose(f);
	      return empty;
	    }
	  total += len;
	}

	//fwrite(buf,1,size,f);

	if(fclose(f))
	{
	  char buf_loc[2048];
	  sprintf(buf_loc,"Write error on %s",out->path().c_str());
	  //gui::syserr(buf);
	  return empty;
	}

	if(total)
		return out;

	return empty;
}
*/
/*
VDir_ptr LogClient::getDir(const char* name)
{
	VDir_ptr empty;

	if(soc_ < 0)
		return empty;

	write(soc_,"list ",5);
	write(soc_,name,strlen(name));
	write(soc_,"\n",1);

	FILE* f = fdopen(soc_,"r");

	char buf[2048];

	//We suppose name is a file name!
	//Create the resulting directory object. We might need to
	//adjust its path later.
	boost::filesystem::path fp(name);
	std::string dirName=fp.parent_path().string();
	VDir_ptr dir(new VDir(dirName));

	//indicates the source of the files
	dir->where(host_ + "@" + port_);

	while(fgets(buf,sizeof(buf),f))
	{
		int mode,uid,gid;
		unsigned int size;
		unsigned int atime,mtime,ctime;
		char name[1024];

		sscanf(buf,"%d %d %d %u %u %u %u %s",
				&mode,&uid,&gid,
				&size,
				&atime,&mtime,&ctime,
				name);

		boost::filesystem::path p(name);
		std::string fileDirName=fp.parent_path().string();
		std::string fileName=p.leaf().string();

		//Adjust the path in the dir
		if(fileDirName != dirName)
		{
			dir->path(fileDirName,false);
		}

		dir->addItem(fileName,size,mtime);
	}

	return dir;
}
*/
