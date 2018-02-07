
install_data(
    join_paths('gamecontrollerdb', 'gamecontrollerdb.txt'),
    install_dir : data_path
)

# NOTE: this only handles the inline version for now
dep_cglm = declare_dependency(
    include_directories: include_directories('cglm/include')
)

taisei_deps += dep_cglm
