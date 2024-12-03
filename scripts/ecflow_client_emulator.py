#!/usr/bin/env python3 -W ignore::DeprecationWarning

#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

import socket
import ssl
import json
import crypt
from pathlib import Path

### 
### This script is a naive emulator for the ecFlow client API, which is used to
### simulate exchanging several types of requests with ecFlow server over a secure
### connection.
###
### It is intended to be used for testing purposes only.
###

host = "<hostname>"
port = <port>
username = "<username>"
password = "<password"
salted = crypt.crypt(password, salt=username)
certdir = Path('.')
certname = "<certificate>"


def exchange(request):

    context = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
    #context.verify_mode = ssl.CERT_OPTIONAL
    context.check_hostname = False
    context.load_verify_locations(certdir / f"{certname}")

    with socket.create_connection((host, port)) as conn:
        with context.wrap_socket(conn, server_hostname=host) as sconn:

            out_payload = json.dumps(request)
            out_header = f"{len(out_payload):08x}"
            outbound = f"{out_header}{out_payload}".encode("utf-8")
            sconn.sendall(outbound)

            inbound = sconn.read(1000000000)
            in_header = inbound[0:8]
            in_payload = inbound[8:]

            # print(f"{out_header}")
            # print(f"{out_payload}")
            # print(f"{in_header}")
            # print(f"{in_payload}")

            response = json.loads(in_payload)
            return response


def process(request):
    response = exchange(request)
    print(f"{json.dumps(request, indent=2)}")
    print(f"{json.dumps(response, indent=2)}")


if __name__ == "__main__":

    #
    # The comprehensive list of commands available at
    #
    # https://ecflow.readthedocs.io/en/latest/client_api/index.html
    #

    requests = {
        "restart": {
            "21ClientToServerRequest": {
                "cmd_": {
                    "polymorphic_id": 2147483649,
                    "polymorphic_name": "CtsCmd",
                    "ptr_wrapper": {
                        "id": 2147483649,
                        "data": {
                            "cereal_class_version": 0,
                            "value0": {
                                "cereal_class_version": 0,
                                "value0": {
                                    "cereal_class_version": 0,
                                    "cl_host_": "iona",
                                },
                                "user_": username,
                                "pswd_": salted,
                                "cu_": True,
                            },
                            "api_": 2,
                        },
                    },
                }
            }
        },
        "ping": {
            "21ClientToServerRequest": {
                "cmd_": {
                    "polymorphic_id": 2147483649,
                    "polymorphic_name": "CtsCmd",
                    "ptr_wrapper": {
                        "id": 2147483649,
                        "data": {
                            "cereal_class_version": 0,
                            "value0": {
                                "cereal_class_version": 0,
                                "value0": {
                                    "cereal_class_version": 0,
                                    "cl_host_": "iona",
                                },
                                "user_": username,
                                "pswd_": salted,
                            },
                            "api_": 8,
                        },
                    },
                }
            }
        },
        "stats": {
            "21ClientToServerRequest": {
                "cmd_": {
                    "polymorphic_id": 2147483649,
                    "polymorphic_name": "CtsCmd",
                    "ptr_wrapper": {
                        "id": 2147483649,
                        "data": {
                            "cereal_class_version": 0,
                            "value0": {
                                "cereal_class_version": 0,
                                "value0": {
                                    "cereal_class_version": 0,
                                    "cl_host_": "iona",
                                },
                                "user_": username,
                                "pswd_": salted,
                            },
                            "api_": 10,
                        },
                    },
                }
            }
        },
        "begin": {
            "21ClientToServerRequest": {
                "cmd_": {
                    "polymorphic_id": 2147483649,
                    "polymorphic_name": "BeginCmd",
                    "ptr_wrapper": {
                        "id": 2147483649,
                        "data": {
                            "cereal_class_version": 0,
                            "value0": {
                                "cereal_class_version": 0,
                                "value0": {
                                    "cereal_class_version": 0,
                                    "cl_host_": "iona",
                                },
                                "user_": username,
                                "pswd_": salted,
                                "cu_": True,
                            },
                            "suiteName_": "new",
                            "force_": False,
                        },
                    },
                }
            }
        },
        "get": {
            "21ClientToServerRequest": {
                "cmd_": {
                    "polymorphic_id": 2147483649,
                    "polymorphic_name": "CtsNodeCmd",
                    "ptr_wrapper": {
                        "id": 2147483649,
                        "data": {
                            "cereal_class_version": 0,
                            "value0": {
                                "cereal_class_version": 0,
                                "value0": {
                                    "cereal_class_version": 0,
                                    "cl_host_": "iona",
                                },
                                "user_": username,
                                "pswd_": salted,
                                "cu_": True,
                            },
                            "api_": 3,
                            "absNodePath_": "",
                        },
                    },
                }
            }
        },
        "log (get)": {
            "21ClientToServerRequest": {
                "cmd_": {
                    "polymorphic_id": 2147483649,
                    "polymorphic_name": "LogCmd",
                    "ptr_wrapper": {
                        "id": 2147483649,
                        "data": {
                            "cereal_class_version": 0,
                            "value0": {
                                "cereal_class_version": 0,
                                "value0": {
                                    "cereal_class_version": 0,
                                    "cl_host_": "iona",
                                },
                                "user_": username,
                                "pswd_": salted,
                                "cu_": True,
                            },
                            "api_": 0,
                            "get_last_n_lines_": 100,
                            "new_path_": "",
                        },
                    },
                }
            }
        },
        "load": {
            "21ClientToServerRequest": {
                "cmd_": {
                    "polymorphic_id": 2147483649,
                    "polymorphic_name": "LoadDefsCmd",
                    "ptr_wrapper": {
                        "id": 2147483649,
                        "data": {
                            "cereal_class_version": 0,
                            "value0": {
                                "cereal_class_version": 0,
                                "value0": {
                                    "cereal_class_version": 0,
                                    "cl_host_": "iona",
                                },
                                "user_": username,
                                "pswd_": salted,
                                "cu_": True,
                            },
                            "force_": True,
                            "defs_": """
#5.13.5
defs_state NET cal_count:0
edit ECF_MICRO '%' # server
edit ECF_HOME '.' # server
edit ECF_JOB_CMD '%ECF_JOB% 1> %ECF_JOBOUT% 2>&1' # server
edit ECF_KILL_CMD 'kill -15 %ECF_RID%' # server
edit ECF_STATUS_CMD 'ps --pid %ECF_RID% -f > %ECF_JOB%.stat 2>&1' # server
edit ECF_URL_CMD '${BROWSER:=firefox} -new-tab %ECF_URL_BASE%/%ECF_URL%' # server
edit ECF_URL_BASE 'https://confluence.ecmwf.int' # server
edit ECF_URL 'display/ECFLOW/ecflow+home' # server
edit ECF_LOG 'iona.3141.ecf.log' # server
edit ECF_INTERVAL '60' # server
edit ECF_LISTS 'iona.3141.ecf.lists' # server
edit ECF_PASSWD 'iona.3141.ecf.passwd' # server
edit ECF_CUSTOM_PASSWD 'iona.3141.ecf.custom_passwd' # server
edit ECF_CHECK 'iona.3141.ecf.check' # server
edit ECF_CHECKOLD 'iona.3141.ecf.check.b' # server
edit ECF_CHECKINTERVAL '120' # server
edit ECF_CHECKMODE 'CHECK_ON_TIME' # server
edit ECF_TRIES '2' # server
edit ECF_VERSION '5.13.5' # server
edit ECF_PORT '3141' # server
edit ECF_HOST 'localhost' # server
suite new
family f
task t
label l \"1\"
meter m 0 60 30
event e
endfamily
endsuite
# enddef""",
                            "defs_filename_": "clone.def",
                        },
                    },
                }
            }
        },
        "delete": {
            "21ClientToServerRequest": {
                "cmd_": {
                    "polymorphic_id": 2147483649,
                    "polymorphic_name": "DeleteCmd",
                    "ptr_wrapper": {
                        "id": 2147483649,
                        "data": {
                            "cereal_class_version": 0,
                            "value0": {
                                "cereal_class_version": 0,
                                "value0": {
                                    "cereal_class_version": 0,
                                    "cl_host_": "iona",
                                },
                                "user_": username,
                                "pswd_": salted,
                                "cu_": True,
                            },
                            "paths_": ["/new"],
                            "force_": False,
                        },
                    },
                }
            }
        },
        "msg": {
            "21ClientToServerRequest": {
                "cmd_": {
                    "polymorphic_id": 2147483649,
                    "polymorphic_name": "LogMessageCmd",
                    "ptr_wrapper": {
                        "id": 2147483649,
                        "data": {
                            "cereal_class_version": 0,
                            "value0": {
                                "cereal_class_version": 0,
                                "value0": {
                                    "cereal_class_version": 0,
                                    "cl_host_": "iona",
                                },
                                "user_": username,
                                "pswd_": salted,
                                "cu_": True,
                            },
                            "msg_": "This is a message in a bottle",
                        },
                    },
                }
            }
        },
        "news": {
            "21ClientToServerRequest": {
                "cmd_": {
                    "polymorphic_id": 2147483649,
                    "polymorphic_name": "CSyncCmd",
                    "ptr_wrapper": {
                        "id": 2147483649,
                        "data": {
                            "cereal_class_version": 0,
                            "value0": {
                                "cereal_class_version": 0,
                                "value0": {
                                    "cereal_class_version": 0,
                                    "cl_host_": "iona",
                                },
                                "user_": username,
                                "pswd_": salted,
                                "cu_": True,
                            },
                            "api_": 0,
                            "client_handle_": 0,
                            "client_state_change_no_": 0,
                            "client_modify_change_no_": 0,
                        },
                    },
                }
            }
        },
        "query (state)": {
            "21ClientToServerRequest": {
                "cmd_": {
                    "polymorphic_id": 2147483649,
                    "polymorphic_name": "QueryCmd",
                    "ptr_wrapper": {
                        "id": 2147483649,
                        "data": {
                            "cereal_class_version": 0,
                            "value0": {
                                "cereal_class_version": 0,
                                "value0": {
                                    "cereal_class_version": 0,
                                    "cl_host_": "iona",
                                },
                                "user_": username,
                                "pswd_": salted,
                                "cu_": True,
                            },
                            "query_type_": "state",
                            "path_to_attribute_": "/new/f/t",
                            "attribute_": "",
                            "path_to_task_": "",
                        },
                    },
                }
            }
        },
        "reloadcustompasswdfile": {
            "21ClientToServerRequest": {
                "cmd_": {
                    "polymorphic_id": 2147483649,
                    "polymorphic_name": "CtsCmd",
                    "ptr_wrapper": {
                        "id": 2147483649,
                        "data": {
                            "cereal_class_version": 0,
                            "value0": {
                                "cereal_class_version": 0,
                                "value0": {
                                    "cereal_class_version": 0,
                                    "cl_host_": "iona",
                                },
                                "user_": username,
                                "pswd_": salted,
                                "cu_": True,
                            },
                            "api_": 18,
                        },
                    },
                }
            }
        },
        "reloadpasswdfile": {
            "21ClientToServerRequest": {
                "cmd_": {
                    "polymorphic_id": 2147483649,
                    "polymorphic_name": "CtsCmd",
                    "ptr_wrapper": {
                        "id": 2147483649,
                        "data": {
                            "cereal_class_version": 0,
                            "value0": {
                                "cereal_class_version": 0,
                                "value0": {
                                    "cereal_class_version": 0,
                                    "cl_host_": "iona",
                                },
                                "user_": username,
                                "pswd_": salted,
                                "cu_": True,
                            },
                            "api_": 16,
                        },
                    },
                }
            }
        },
        "reloadwhitelistfile": {
            "21ClientToServerRequest": {
                "cmd_": {
                    "polymorphic_id": 2147483649,
                    "polymorphic_name": "CtsCmd",
                    "ptr_wrapper": {
                        "id": 2147483649,
                        "data": {
                            "cereal_class_version": 0,
                            "value0": {
                                "cereal_class_version": 0,
                                "value0": {
                                    "cereal_class_version": 0,
                                    "cl_host_": "iona",
                                },
                                "user_": username,
                                "pswd_": salted,
                                "cu_": True,
                            },
                            "api_": 6,
                        },
                    },
                }
            }
        },
        "requeue": {
            "21ClientToServerRequest": {
                "cmd_": {
                    "polymorphic_id": 2147483649,
                    "polymorphic_name": "RequeueNodeCmd",
                    "ptr_wrapper": {
                        "id": 2147483649,
                        "data": {
                            "cereal_class_version": 0,
                            "value0": {
                                "cereal_class_version": 0,
                                "value0": {
                                    "cereal_class_version": 0,
                                    "cl_host_": "iona",
                                },
                                "user_": username,
                                "pswd_": salted,
                                "cu_": True,
                            },
                            "paths_": ["/new"],
                            "option_": 0,
                        },
                    },
                }
            }
        },
        "server_version": {
            "21ClientToServerRequest": {
                "cmd_": {
                    "polymorphic_id": 2147483649,
                    "polymorphic_name": "ServerVersionCmd",
                    "ptr_wrapper": {
                        "id": 2147483649,
                        "data": {
                            "cereal_class_version": 0,
                            "value0": {
                                "cereal_class_version": 0,
                                "value0": {
                                    "cereal_class_version": 0,
                                    "cl_host_": "iona",
                                },
                                "user_": username,
                                "pswd_": salted,
                                "cu_": True,
                            },
                        },
                    },
                }
            }
        },
        "suites": {
            "21ClientToServerRequest": {
                "cmd_": {
                    "polymorphic_id": 2147483649,
                    "polymorphic_name": "CtsCmd",
                    "ptr_wrapper": {
                        "id": 2147483649,
                        "data": {
                            "cereal_class_version": 0,
                            "value0": {
                                "cereal_class_version": 0,
                                "value0": {
                                    "cereal_class_version": 0,
                                    "cl_host_": "iona",
                                },
                                "user_": username,
                                "pswd_": salted,
                                "cu_": True,
                            },
                            "api_": 11,
                        },
                    },
                }
            }
        },
    }

    process(requests["restart"])
    process(requests["ping"])
    process(requests["reloadcustompasswdfile"])
    process(requests["reloadpasswdfile"])
    process(requests["reloadwhitelistfile"])
    process(requests["stats"])
    process(requests["msg"])
    process(requests["log (get)"])
    process(requests["delete"])
    process(requests["load"])
    process(requests["begin"])
    process(requests["query (state)"])
    process(requests["news"])
    process(requests["get"])
    process(requests["server_version"])
    process(requests["suites"])
