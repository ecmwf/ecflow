
#include <ostream>
#include <string>

#include "nlohmann/json.hpp"

class TokenFile {
  public:
	TokenFile() = delete;
	TokenFile(const std::string& tokenfile);
    ~TokenFile();
  private:
    std::string tokenfile_;
};

inline
TokenFile::TokenFile(const std::string& tokenfile) : tokenfile_(tokenfile) {
    // write api token file
    // api-keys: 3a8c3f7ac204d9c6370b5916bd8b86166c208e10776285edcbc741d56b5b4c1e
    //           351db772d94310a6d57aa7144448f4c108e7ee2e2a00a74edbdf8edb11bee71b
    //           764073a74875ada28859454e58881229a5149ae400589fc617234d8d96c6d91a
    //           5c6f6a003f3292c4d7671c9ad8ca10fb76e2273e584992f0d1f8fdf4abcdc81e

    nlohmann::json j = {
        { {"hash", "sha256$22660ab1789dc30e$7e24f61129294505b6ac310ebe891df9800f4854a67bac953bb86bf9fd726813"}, {"description", "test-app-1"}, {"expires_at", ""} },
        { {"hash", "pbkdf2:sha256:20000$Iqbh8Bz86hYHpkpn$ea95e8fb276c602fe4a4b56569fbcc321be5b31a3caa6bb3a9001e595349887f"}, {"description", "test-app-2"}, {"expires_at", "2100-01-01T00:00:00Z"} },
        { {"hash", "pbkdf2:sha256:20000$Iqbh8Bz86hYHpkpn$ea95e8fb276c602fe4a4b56569fbcc321be5b31a3caa6bb3a9001e595349887f"}, {"description", "test-app-3"}, {"expires_at", "2000-01-01T00:00:00Z"} },
        { {"hash", "pbkdf2:sha256:20000$Iqbh8Bz86hYHpkpn$ea95e8fb276c602fe4a4b56569fbcc321be5b31a3caa6bb3a9001e595349887f"}, {"description", "test-app-4"}, {"revoked_at", "2000-01-01T00:00:00Z"} }
    };

    std::ofstream o(tokenfile_);
    o << std::setw(4) << j << std::endl;
    setenv("ECF_API_TOKEN_FILE", tokenfile_.c_str(), 1);
}

inline
TokenFile::~TokenFile() {
	if (remove(tokenfile_.c_str()) != 0) {
		std::cerr << "Failed to remove token file " << tokenfile_ << std::endl;
	}
	else {
	    BOOST_TEST_MESSAGE("Removed token file " << tokenfile_);
	}
}
