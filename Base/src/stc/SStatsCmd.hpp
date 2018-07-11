#ifndef SSTATS_CMD_HPP_
#define SSTATS_CMD_HPP_
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// Name        :
// Author      : Avi
// Revision    : $Revision: #8 $ 
//
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0 
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0. 
// In applying this licence, ECMWF does not waive the privileges and immunities 
// granted to it by virtue of its status as an intergovernmental organisation 
// nor does it submit to any jurisdiction. 
//
// Description :
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

#include "ServerToClientCmd.hpp"
#include "Stats.hpp"
class AbstractServer;

//================================================================================
// Paired with CtsCmd(SERVER_STATS)
// Client---(CtsCmd(SERVER_STATS))---->Server-----(SStatsCmd)--->client:
// ****** Used in Test ONLY, since Stats is subject to change for each release
// ****** see ECFLOW-880, we use CtsCmd(STATS) to return server stats as a string
// ****** this allows the format to change in the server(with out affecting protocol)
//================================================================================
class SStatsCmd : public ServerToClientCmd {
public:
   explicit SStatsCmd(AbstractServer* as );
	SStatsCmd() : ServerToClientCmd() {}

	void init(AbstractServer* as);

	virtual std::ostream& print(std::ostream& os) const;
	virtual bool equals(ServerToClientCmd*) const;
	virtual bool handle_server_response( ServerReply& server_reply, Cmd_ptr cts_cmd, bool debug ) const;

private:
 	Stats stats_;

   friend class cereal::access;
   template<class Archive>
   void serialize(Archive & ar, std::uint32_t const version )
   {
 		ar(cereal::base_class< ServerToClientCmd >( this ),
 		   CEREAL_NVP(stats_));
  	}
};

std::ostream& operator<<(std::ostream& os, const SStatsCmd&);

#endif
