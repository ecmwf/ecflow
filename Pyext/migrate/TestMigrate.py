
import os
import ecflow_migrate
import unittest
import filecmp

# These tests the migration for  ecflow < 318 to ecflow 318
#
# In ecflow_client --migrate, we can get abort reason to span multiple line, hence messing up load
#
class TestMigrate318(unittest.TestCase):
    def setUp(self):
        # perform setup actions if any
        pass
    def tearDown(self):
        # Perform clean -up actions if any
        pass
    
    def testMigrateVersionNumber(self):
        list_of_defs_lines =[ "# 3.1.2"]

        mig = ecflow_migrate.Migrator(list_of_defs_lines)
        self.assertEqual(mig.version_number_as_integer(0), 312,"expected version 3.1.2")

        list_of_defs_lines =[ "# "]
        mig = ecflow_migrate.Migrator(list_of_defs_lines)
        self.assertEqual(mig.version_number_as_integer(10), 10,"Expected 10, since valid version not provided")

    def test_no_migration(self):
        migration_count = ecflow_migrate.do_migrate("migrate/no_migration.def")
        self.assertEqual(migration_count,0,"Expected no migration")
        self.assertTrue(filecmp.cmp("migrate/no_migration.def","migrate/no_migration.mig"))
        # remove the generated file
        os.remove("migrate/no_migration.mig")
 
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
        migration_count = ecflow_migrate.do_migrate("migrate/aborted_reason_bug.def")
        self.assertEqual(migration_count,1,"Expected defs file to be migrated")
                 
        # remove the generated file
        os.remove("migrate/aborted_reason_bug.mig")
         
# # ===============================================================================================
 
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
 
    def test_migrate_label_file(self):
        migration_count = ecflow_migrate.do_migrate("migrate/label_bug.def")
        self.assertEqual(migration_count,1,"Expected defs file to be migrated")
                 
        # remove the generated file
        os.remove("migrate/label_bug.mig")
 
# ====================================================================================================
# test both together
 
    def test_migrate_abort_and_label(self):
        migration_count = ecflow_migrate.do_migrate("migrate/abort_and_label_bug.def")
        self.assertEqual(migration_count,2,"Expected 2 migrations, one for abort and one for label")
                 
        # remove the generated file
        os.remove("migrate/abort_and_label_bug.mig")
         


#run the tests 
if __name__  == '__main__':
    #print "Current working directory: " + os.getcwd()
    unittest.main()