//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

std::vector<ViewProfile*> ViewProfile::profiles_;

ViewProfile::ViewProfile()
{
	/*if(access(configDir_.c_str(),F_OK) != 0 )
	{
			if(mkdir(configDir_.c_str(),0777) == -1)
			{
					//error
			}
		}*/

}

//Static
ViewProfile* ViewProfile::load()
{


}

//static

ViewProfile* ViewProfile::load(const std::string& name)
{
	//Creates the top level profiles dir
	std::string path(ViewConfig::Instance()->configDir());
	path+="/profiles";

	if(access(path.c_str(),F_OK) != 0 )
	{
		if(mkdir(path.c_str(),0777) == -1)
		{
						//error
		}
	}

	//
	path_=path + "/" + name;




	std::ifstream in(path.c_str());
	if(!in.good())
		return false;

	std::string line;
	while(getline(in,line))
	{
		//We ignore comment lines
		std::string buf=boost::trim_left_copy(line);
		if(buf.size() > 1 && buf.at(0) == '#')
			continue;

		std::vector<std::string> sv;
		boost::split(sv,line,boost::is_any_of(","));

		if(sv.size() >= 3)
		{
						add(sv[0],sv[1],sv[2]);
		}
	}

	in.close();

	return true;

}
