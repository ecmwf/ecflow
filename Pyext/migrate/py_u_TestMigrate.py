
import os
import ecflow_migrate
import unittest
import filecmp

def get_parent_dir(file_path):
    return os.path.dirname(file_path)

def get_root_source_dir():
    cwd = os.getcwd()
    #print "get_root_source_dir from: " + cwd
    while (1):
        # Get to directory that has ecflow
        head, tail = os.path.split(cwd)
        print "   head:" + head
        print "   tail:" + tail
        if tail.find("ecflow") != -1 :
            
            # bjam, already at the source directory
            if os.path.exists(cwd + "/VERSION.cmake"): 
                print "   Found VERSION.cmake in " + cwd
                return cwd
        
        if tail != "Pyext" and tail != "migrate":
            # in cmake, we may be in the build directory, hence we need to determine source directory
            file = cwd + "/CTestTestfile.cmake"
            #print "   searching for " + file
            if os.path.exists(file):
                # determine path by looking into this file:
                for line in open(file):
                    ## Source directory: /tmp/ma0/clientRoot/workspace/working-directory/ecflow/Acore
                    if line.find("Source directory"):
                        tokens = line.split()
                        if len(tokens) == 4:
                            #print "   returning root_source_dir:", tokens[3]
                            return tokens[3]
                raise RuntimeError("ERROR could not find Source directory in CTestTestfile.cmake")
            else:
                raise RuntimeError("ERROR could not find file CTestTestfile.cmake in " + cwd)
                
        cwd = head
    return cwd



# These tests the migration for  ecflow < 318 to ecflow 318
#
# In ecflow_client --migrate, we can get abort reason to span multiple line, hence messing up load
#
class TestMigrate318(unittest.TestCase):
    def setUp(self):
        # perform setup actions if any
        self.workspace_dir = get_root_source_dir()
        #print "setup : " + self.workspace_dir
        
    def tearDown(self):
        # Perform clean -up actions if any
        pass
    
    def locate(self,file):
        return self.workspace_dir + "/Pyext/" + file
    
    def testMigrateVersionNumber(self):
        list_of_defs_lines =[ "# 3.1.2"]
  
        mig = ecflow_migrate.Migrator(list_of_defs_lines)
        self.assertEqual(mig.version_number_as_integer(0), 312,"expected version 3.1.2")
  
        list_of_defs_lines =[ "# "]
        mig = ecflow_migrate.Migrator(list_of_defs_lines)
        self.assertEqual(mig.version_number_as_integer(10), 10,"Expected 10, since valid version not provided")
  
    def test_no_migration(self):
        migration_count = ecflow_migrate.do_migrate(self.locate("migrate/no_migration.def"))
        self.assertEqual(migration_count,0,"Expected no migration")
        self.assertTrue(filecmp.cmp(self.locate("migrate/no_migration.def"),self.locate("migrate/no_migration.mig")))
        # remove the generated file
        try: os.remove(self.locate("migrate/no_migration.mig"))
        except: pass
   
# ==============================================================================================
    # Test for abort bug
       
    def test_normal_abort_case(self):
        list_of_defs_lines = [ "# 3.1.2", "task task abort<: aa >abort\n" ]      
        abort_migrator = ecflow_migrate.MigrateForTaskAbort(list_of_defs_lines)
        abort_migrator.migrate(318)
          
        expected_output_lines = ["# 3.1.2", "task task abort<: aa >abort\n"]
        self.assertEqual(abort_migrator.output_lines(),expected_output_lines)
    
    def test_task_abort_reason_bug_simple(self):
        list_of_defs_lines = ["# 3.1.2", "task task abort<: aa\n",">abort\n" ] 
        abort_migrator = ecflow_migrate.MigrateForTaskAbort(list_of_defs_lines)
        abort_migrator.migrate(318)
       
        expected_output_lines = ["# 3.1.2","task task abort<: aa >abort\n"]
        self.assertEqual(abort_migrator.output_lines(),expected_output_lines)
   
    def test_task_abort_reason_bug(self):
        # In ecflow_client --migrate, we can get abort reason to span multiple line, hence messing up load
        list_of_defs_lines = ["# 3.1.2","task task # passwd:jxX0gIbR abort<:   Script for /suite/family/task can not be found:\n",
                              "line2\n", "line3\n",">abort try:2 state:aborted dur:02:19:57 flag:task_aborted,no_script\n" ]
           
        abort_migrator = ecflow_migrate.MigrateForTaskAbort(list_of_defs_lines)
        abort_migrator.migrate(318)
            
        expected_output_lines = ["# 3.1.2","task task # passwd:jxX0gIbR abort<:   Script for /suite/family/task can not be found: line2 line3 >abort try:2 state:aborted dur:02:19:57 flag:task_aborted,no_script\n"]
        self.assertEqual(abort_migrator.output_lines(),expected_output_lines)
            
    def test_migrate_abort_file(self):
        migration_count = ecflow_migrate.do_migrate(self.locate("migrate/aborted_reason_bug.def"))
        self.assertEqual(migration_count,1,"Expected defs file to be migrated")
                   
        # remove the generated file
        try: os.remove(self.locate("migrate/aborted_reason_bug.mig"))
        except: pass
           
# ===============================================================================================
   
    def test_normal_label(self):
        list_of_defs_lines = ["# 3.1.2","label name \"value\"\n" ]      
  
        migrator = ecflow_migrate.MigrateForLabel(list_of_defs_lines)
        migrator.migrate(319)
      
        expected_output_lines = ["# 3.1.2","label name \"value\"\n" ]
        self.assertEqual(migrator.output_lines(),expected_output_lines)
   
    def test_label_over_multiple_lines(self):
        list_of_defs_lines = ["# 3.1.2","label name \"value1\n", "value2\n", "value3\"\n" ]  
          
        migrator = ecflow_migrate.MigrateForLabel(list_of_defs_lines)
        migrator.migrate(319)
      
        expected_output_lines = ["# 3.1.2","label name \"value1value2value3\"\n" ]
        self.assertEqual(migrator.output_lines(),expected_output_lines)
          
    def test_label_over_multiple_lines_with_state(self):
        list_of_defs_lines = ["# 3.1.2","label name \"value1\n", "value2\n", "value3\" # \"value4\n", "value5\"\n" ]  
          
        migrator = ecflow_migrate.MigrateForLabel(list_of_defs_lines)
        migrator.migrate(319)
      
        expected_output_lines = ["# 3.1.2","label name \"value1value2value3\" # \"value4value5\"\n" ]
        self.assertEqual(migrator.output_lines(),expected_output_lines)
   
   
    def test_migrate_label_file(self):
        migration_count = ecflow_migrate.do_migrate(self.locate("migrate/label_bug.def"))
        self.assertEqual(migration_count,1,"Expected defs file to be migrated")
                   
        # remove the generated file
        try: os.remove(self.locate("migrate/label_bug.mig"))
        except: pass
  
    def test_migrate_variable_file(self):
        migration_count = ecflow_migrate.do_migrate(self.locate("migrate/variable_bug.def"))
        self.assertEqual(migration_count,1,"Expected defs file to be migrated")
                   
        # remove the generated file
        try: os.remove(self.locate("migrate/variable_bug.mig"))
        except: pass
   
# ====================================================================================================
# test both together
   
    def test_migrate_abort_and_label(self):
        migration_count = ecflow_migrate.do_migrate(self.locate("migrate/abort_and_label_bug.def"))
        self.assertEqual(migration_count,2,"Expected 2 migrations, one for abort and one for label")
                   
        # remove the generated file
        try: os.remove(self.locate("migrate/abort_and_label_bug.mig"))
        except: pass
           
#====================================================================================================
#test history
 
    def test_migrate_history(self):
        print "test_migrate_history"
        migration_count = ecflow_migrate.do_migrate(self.locate("migrate/history_bug.def"))
        self.assertEqual(migration_count,1,"Expected 1 migrations,for history")
                 
        # remove the generated file
        try: os.remove(self.locate("migrate/history_bug.mig"))
        except: pass

#run the tests 
if __name__  == '__main__':
    #print "Current working directory: " + os.getcwd()
    unittest.main()