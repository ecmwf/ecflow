/*
 * Copyright 2009- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

#include <boost/python.hpp>
#include <boost/python/raw_function.hpp>

namespace bp = boost::python;

#include "ecflow/core/CheckPt.hpp"
#include "ecflow/core/DState.hpp"
#include "ecflow/core/Ecf.hpp"
#include "ecflow/core/File.hpp"
#include "ecflow/core/PrintStyle.hpp"
#include "ecflow/core/SState.hpp"
#include "ecflow/core/TimeSeries.hpp"
#include "ecflow/core/TimeSlot.hpp"
#include "ecflow/python/BoostPythonUtil.hpp"
#include "ecflow/python/Edit.hpp"
#include "ecflow/python/NodeAttrDoc.hpp"

// See: http://wiki.python.org/moin/boost.python/HowTo#boost.function_objects
template <class K, class T>
struct pair_to_tuple
{
    typedef pair_to_tuple<K, T> converter;
    typedef std::pair<K, T> ctype;

    static PyObject* convert(ctype const& v) { return bp::incref(bp::make_tuple(v.first, v.second).ptr()); }
    static void register_to_python() { bp::to_python_converter<ctype, converter>(); }
};

bool debug_build() {
#ifdef NDEBUG
    return false;
#else
    return true;
#endif
}

void export_Core() {
    // For use in test only
    bp::def("debug_build", debug_build);

    // see: https://github.com/boostorg/python/blob/master/test/raw_ctor.cpp
    // Uses a raw constructor approach to support pass arbitrary number arguments on the python side.
    // using no_init postpones defining __init__ function until after raw_function for proper overload resolution order,
    // since later defs get higher priority.
    bp::class_<Edit>("Edit", NodeAttrDoc::variable_doc(), bp::no_init)
        .def("__init__", raw_function(&Edit::init, 0)) // raw_constructor -> will call -> def(init<dict>() )
        .def(bp::init<bp::dict>())                     //
        .def(bp::init<bp::dict, bp::dict>())           //
        .def("__str__", &Edit::to_string)              // __str__
        ;

    bp::class_<ecf::File, boost::noncopyable>("File", "Utility class, Used in test only.", bp::no_init)
        .def("find_server", &ecf::File::find_ecf_server_path, "Provides pathname to the server")
        .staticmethod("find_server")
        .def("find_client", &ecf::File::find_ecf_client_path, "Provides pathname to the client")
        .staticmethod("find_client")
        .def("source_dir", &ecf::File::root_source_dir, "Path name to ecflow source directory")
        .staticmethod("source_dir")
        .def("build_dir", &ecf::File::root_build_dir, "Path name to ecflow build directory")
        .staticmethod("build_dir");

    bp::enum_<PrintStyle::Type_t>(
        "Style",
        "Style is used to control printing output for the definition\n\n"
        "- DEFS:  This style outputs the definition file in a format that is parse-able.\n"
        "         and can be re-loaded back into the server.\n"
        "         Externs are automatically added.\n"
        "         This excludes the edit history.\n"
        "- STATE: The output includes additional state information for debug\n"
        "         This excludes the edit history\n"
        "- MIGRATE: Output includes structure and state, allow migration to future ecflow versions\n"
        "         This includes edit history. If file is reloaded no checking is done\n\n"
        "The following shows a summary of the features associated with each choice\n"
        "\n"
        "   ===================== ==== ===== =======\n"
        "   Functionality         DEFS STATE MIGRATE\n"
        "   ===================== ==== ===== =======\n"
        "   Auto generate externs  Yes  Yes   No\n"
        "   Checking on reload     Yes  Yes   No\n"
        "   Edit History           No   No    Yes\n"
        "   Show trigger AST       No   Yes   No\n"
        "   ===================== ==== ===== =======\n")
        .value("NOTHING", PrintStyle::NOTHING)
        .value("DEFS", PrintStyle::DEFS)
        .value("STATE", PrintStyle::STATE)
        .value("MIGRATE", PrintStyle::MIGRATE);

    bp::class_<PrintStyle, boost::noncopyable>(
        "PrintStyle",
        "Singleton used to control the print Style. See :py:class:`ecflow.Style`\n\n"
        "\nUsage::\n\n"
        "   old_style = PrintStyle.get_style()\n"
        "   PrintStyle.set_style(PrintStyle.STATE)\n"
        "   ...\n"
        "   print(defs)                     # show the node state\n"
        "   PrintStyle.set_style(old_style) # reset previous style\n",
        bp::no_init)
        .def("get_style", &PrintStyleHolder::getStyle, "Returns the style, static method")
        .staticmethod("get_style")
        .def("set_style", &PrintStyleHolder::setStyle, "Set the style, static method")
        .staticmethod("set_style");

    bp::enum_<ecf::CheckPt::Mode>(
        "CheckPt",
        "CheckPt is enum that is used to control check pointing in the `ecflow_server`_\n\n"
        "- NEVER  : Switches of check pointing\n"
        "- ON_TIME: `check point`_ file is saved periodically, specified by checkPtInterval. This is the default.\n"
        "- ALWAYS : `check point`_ file is saved after any state change, *not* recommended for large definitions\n"
        "- UNDEFINED : None of the the above, used to provide default argument\n")
        .value("NEVER", ecf::CheckPt::NEVER)
        .value("ON_TIME", ecf::CheckPt::ON_TIME)
        .value("ALWAYS", ecf::CheckPt::ALWAYS)
        .value("UNDEFINED", ecf::CheckPt::UNDEFINED);

    bp::class_<Ecf, boost::noncopyable>("Ecf", "Singleton used to control ecf debugging\n\n", bp::no_init)
        .def("debug_equality", &Ecf::debug_equality, "Returns true if debugging of equality is enabled")
        .staticmethod("debug_equality")
        .def("set_debug_equality", &Ecf::set_debug_equality, "Set debugging for equality")
        .staticmethod("set_debug_equality")
        .def("debug_level",
             &Ecf::debug_level,
             "Returns integer showing debug level. debug_level > 0 will disable some warning messages")
        .staticmethod("debug_level")
        .def("set_debug_level",
             &Ecf::set_debug_level,
             "Set debug level. debug_level > 0 will disable some warning messages")
        .staticmethod("set_debug_level");

    bp::enum_<NState::State>(
        "State",
        "Each `node`_ can have a status, which reflects the life cycle of a node.\n\n"
        "It varies as follows:\n\n"
        "- When the definition file is loaded into the `ecflow_server`_ the `task`_ status is `unknown`_\n"
        "- After begin command the `task`_ s are either `queued`_, `complete`_, `aborted`_ or `suspended`_ ,\n"
        "  a suspended task means that the task is really `queued`_ but it must be resumed by\n"
        "  the user first before it can be `submitted`_. See :py:class:`ecflow.DState`\n"
        "- Once the `dependencies`_ are resolved a task is submitted and placed into the `submitted`_ state,\n"
        "  however if the submission fails, the task is placed in a `aborted`_ state.\n"
        "- On a successful submission the task is placed into the `active`_ state\n"
        "- Before a job ends, it may send other message to the server such as:\n"
        "  Set an `event`_, Change a `meter`_, Change a `label`_, send a message to log file\n\n"
        "Jobs end by becoming either `complete`_ or `aborted`_")
        .value("unknown", NState::UNKNOWN)
        .value("complete", NState::COMPLETE)
        .value("queued", NState::QUEUED)
        .value("aborted", NState::ABORTED)
        .value("submitted", NState::SUBMITTED)
        .value("active", NState::ACTIVE);

    bp::enum_<DState::State>("DState",
                             "A DState is like a ecflow.State, except for the addition of SUSPENDED\n\n"
                             "Suspended stops job generation, and hence is an attribute of a Node.\n"
                             "DState can be used for setting the default state of node when it is\n"
                             "begun or re queued. DState is used for defining `defstatus`_.\n"
                             "See :py:class:`ecflow.Node.add_defstatus` and :py:class:`ecflow.Defstatus`\n"
                             "The default state of a `node`_ is `queued`_.\n"
                             "\nUsage::\n\n"
                             "   task = ecflow.Task('t1')\n"
                             "   task.add_defstatus(ecflow.DState.complete)"
                             "   task = ecflow.Task('t2')\n"
                             "   task += Defstatus('complete')\n"
                             "   task = Task('t3',\n"
                             "               Defstatus('complete')) # create in place\n")
        .value("unknown", DState::UNKNOWN)
        .value("complete", DState::COMPLETE)
        .value("queued", DState::QUEUED)
        .value("aborted", DState::ABORTED)
        .value("submitted", DState::SUBMITTED)
        .value("suspended", DState::SUSPENDED)
        .value("active", DState::ACTIVE);

    bp::class_<Defstatus>("Defstatus",
                          "A `node`_ can be set with a default status other the `queued`_\n\n"
                          "The default state of a `node`_ is `queued`_.\n"
                          "This defines the state to take at 'begin' or 're-queue' time\n"
                          "See :py:class:`ecflow.Node.add_defstatus` and :py:class:`ecflow.DState`\n",
                          bp::init<DState::State>())
        .def(bp::init<std::string>()) // constructor
        .def("state", &Defstatus::state)
        .def("__str__", &Defstatus::to_string) // __str__
        ;

    bp::enum_<SState::State>("SState",
                             "A SState holds the `ecflow_server`_ state\n\n"
                             "See `server states`_")
        .value("HALTED", SState::HALTED)
        .value("SHUTDOWN", SState::SHUTDOWN)
        .value("RUNNING", SState::RUNNING);

    bp::class_<ecf::TimeSlot>("TimeSlot",
                              "Represents a time slot.\n\n"
                              "It is typically used as an argument to a :py:class:`TimeSeries` or\n"
                              "other time dependent attributes of a node.\n"
                              "\n"
                              "\nConstructor::\n\n"
                              "   TimeSlot(hour,min)\n"
                              "      int hour:   represent an hour:\n"
                              "      int minute: represents a minute:\n"
                              "\nUsage::\n\n"
                              "   ts = TimeSlot(10,11)\n",
                              bp::init<int, int>())
        .def("__str__", &ecf::TimeSlot::toString)   // __str__
        .def("__copy__", copyObject<ecf::TimeSlot>) // __copy__ uses copy constructor
        .def(bp::self == bp::self)                  // __eq__
        .def("hour", &ecf::TimeSlot::hour)          // return int
        .def("minute", &ecf::TimeSlot::minute)      // return int
        .def("empty", &ecf::TimeSlot::isNULL)       // return bool
        ;

    // single slot, | start, finish, incr,  bool relative to suite start
    bp::class_<ecf::TimeSeries>(
        "TimeSeries",
        "A TimeSeries can hold a single time slot or a series.\n\n"
        "Time series can be created relative to the `suite`_ start or start of a repeating node.\n"
        "A Time series can be used as argument to the :py:class:`ecflow.Time`, :py:class:`ecflow.Today` and "
        ":py:class:`ecflow.Cron` attributes of a node.\n"
        "If a time the job takes to complete is longer than the interval, a 'slot' is missed\n"
        "e.g time 10:00 20:00 01:00, if the 10.00 run takes more than an hour the 11.00 is missed\n\n"
        "\nConstructor::\n\n"
        "   TimeSeries(single,relative_to_suite_start)\n"
        "      TimeSlot single :  A single point in a 24 clock \n"
        "      optional bool relative_to_suite_start : depend on suite begin time or\n"
        "                                              start of repeating node. Default is false\n"
        "\n"
        "   TimeSeries(hour,minute,relative_to_suite_start)\n"
        "      int hour   :  hour in 24 clock \n"
        "      int minute :  minute < 59 \n"
        "      bool relative_to_suite_start<optional> : depend on suite begin time or\n"
        "                                              start of repeating node. Default is false\n"
        "\n"
        "   TimeSeries(start,finish,increment,relative_to_suite_start)\n"
        "      start TimeSlot :     The start time  \n"
        "      finish TimeSlot :    The finish time, when used in a series. This must greater than the start.\n"
        "      increment TimeSlot : The increment. This must be less that difference between start and finish\n"
        "      bool relative_to_suite_start<optional> : The time is relative suite start, or start of repeating node.\n"
        "                                               The default is false\n"
        "\nExceptions:\n\n"
        "- Raises IndexError when an invalid time series is specified\n"
        "\nUsage::\n\n"
        "   time_series = TimeSeries(TimeSlot(10,11),False)\n",
        bp::init<ecf::TimeSlot, boost::python::optional<bool>>())
        .def(bp::init<int, int, boost::python::optional<bool>>())
        .def(bp::init<ecf::TimeSlot, ecf::TimeSlot, ecf::TimeSlot, boost::python::optional<bool>>())
        .def(bp::self == bp::self)                    // __eq__
        .def("__str__", &ecf::TimeSeries::toString)   // __str__
        .def("__copy__", copyObject<ecf::TimeSeries>) // __copy__ uses copy constructor
        .def("has_increment",
             &ecf::TimeSeries::hasIncrement,
             "distinguish between a single time slot and a series. returns true for a series") // false if single time
                                                                                               // slot
        .def("start",
             &ecf::TimeSeries::start,
             bp::return_value_policy<bp::copy_const_reference>(),
             "returns the start time") // returns a time slot
        .def("finish",
             &ecf::TimeSeries::finish,
             bp::return_value_policy<bp::copy_const_reference>(),
             "returns the finish time if time series specified, else returns a NULL time slot") // returns a time slot
        .def("incr",
             &ecf::TimeSeries::incr,
             bp::return_value_policy<bp::copy_const_reference>(),
             " returns the increment time if time series specified, else returns a NULL time slot") // returns a time
                                                                                                    // slot
        .def("relative",
             &ecf::TimeSeries::relative,
             "returns a boolean where true means that the time series is relative");

    pair_to_tuple<std::string, std::string>::register_to_python();
}
