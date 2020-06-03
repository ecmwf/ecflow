# find all cpp file that have std::runtime_error.
# if these file dont have #include <stdexcept> add it as the first include

import os,fnmatch

def all_files(root, patterns='*', single_level=False, yield_folders=False):
    """Expand patterns from semi-colon separated string to list"""
    patterns = patterns.split(';')
    for path, subdirs, files in os.walk(root):
        if yield_folders:
            files.extend(subdirs)
        files.sort()
        for name in files:
            for pattern in patterns:
                if fnmatch.fnmatch(name,pattern):
                    yield os.path.join(path, name)
                    break
        if single_level:
            break    
        
for cpp_file in all_files('.', '*.cpp'):
    if 'cereal' in cpp_file:
        continue
    #print(cpp_file)
    file_obj = open(cpp_file,'r')
    try:
        list_of_all_lines = file_obj.readlines()
        
        has_runtime_error = False
        has_correct_include = False
        for line in list_of_all_lines:
            if 'std::runtime_error' in line:
                has_runtime_error = True
            if '#include <stdexcept>' in line:
                has_correct_include = True
                
        if has_runtime_error:
            #print(cpp_file, " uses std::runtime_error ", has_correct_include )
            if not has_correct_include:
                print(cpp_file," modifying")
                # add #include <stdexcept>
                added_include = False
                new_list = []
                for line in list_of_all_lines:
                    if not added_include and '#include' in line:
                        new_list.append('#include <stdexcept>\n')
                        added_include = True
                    new_list.append(line)
                    
                changed_file = open(cpp_file,'w')
                changed_file.writelines(new_list)
                changed_file.close()
    finally:
        file_obj.close()
        