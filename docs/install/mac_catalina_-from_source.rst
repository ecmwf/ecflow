.. _mac_catalina_-from_source:

Mac(Catalina)-from source
/////////////////////////


The install relies on brew. please see https://brew.sh, by default using
the apple clang compiler. Alternatively, we can use gnu, but this
requires additional steps as we need to build boost ourselves.

-  first, install brew. Paste that in a macOS Terminal or Linux shell
   prompt.

.. note::

 /bin/bash -c "$(curl -fsSL                                            
 https://raw.githubusercontent.com/Homebrew/install/master/install.sh  
 )"                                                                    

-  next, install ecflow dependencies

.. note::

 brew install cmake                                                    
                                                                       
 brew install boost                                                    
                                                                       
 brew install boost-python3                                            
                                                                       
 brew install openssl                                                  
                                                                       
 brew install qt                                                       
                                                                       
 brew install figlet # replacement for banner in scripts (i.e. .ecf    
 files)                                                                

-  clone and place in $HOME/git

.. note::

 git clone ssh://git@git.ecmwf.int/ecflow/ecflow.git                   
                                                                       
 git clone ssh://git@git.ecmwf.int/ecsdk/ecbuild.git                   
                                                                       
 git clone ssh://git@git.ecmwf.int/ecsdk/metabuilder.git # optional    

-  build ecflow, and install to ${HOME}/install/ecflow/${version}

.. note::

 cd $HOME/git/ecflow                                                   
                                                                       
 git checkout develop                                                  
                                                                       
 sh -x build_scripts/mac.sh make -j8 install # if you make a mistake   
 use: sh -x build_scripts/mac.sh clean make -j8 install # clean will   
 blast build directory                                                 

-  Add the following to your **.bash_profile**

.. note::

 ecflow_version=$(awk '/^project/ && /ecflow/ && /VERSION/ {for        
 (I=1;I<=NF;I++) if ($I == "VERSION") {print $(I+1)};}'                
 $HOME/git/ecflow/CMakeLists.txt)                                      
                                                                       
 python_dot_version=$(python3 -c 'import                               
 sys;print(sys.version_info[0],".",sys.version_info[1],sep="")')       
                                                                       
 export PATH="$PATH:$HOME/install/ecflow/$ecflow_version/bin"          
                                                                       
 export                                                                
 PYTHONPATH="$HOME/install/                                            
 ecflow/$ecflow_version/lib/python${python_dot_version}/site-packages" 
                                                                       
 export WK='$HOME/git/ecflow'                                          
                                                                       
                                                                       
                                                                       
 # to use the meta builder on MAC:                                     
                                                                       
 export PERM='$HOME/PERM'                                              
                                                                       
 export SCRATCH='$HOME/SCRATCH'                                        
                                                                       
 export METABUILDER_TESTING=1                                          

-  To use the meta builder on mac: Assumes you have updated
   .bash_profile.

.. note::

 cd $WK                                                                
                                                                       
 # This will kill the server if its already running, then start the    
 server on port 4141                                                   
                                                                       
 # Run the metabuilder/develop                                         
                                                                       
 # start the GUI                                                       
                                                                       
 sh -x build_scripts/nightly/quick_install.sh                          

.. note::

 You will only be able to run the apple_clang compiler in the          
 metabuilder, to use gnu.92 you will need to install/build with this   
 compiler.                                                             
