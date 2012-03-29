require 'rbconfig'

module_search_path '../thirdparty/modules'

# extend lace with some project specific functions:
if !$defbuild1_configuration_motor3d
	$defbuild1_configuration_motor3d = true

	LaceModule.class_eval do	
        alias_method :old_run_section, :run_section

        def run_section( section )
            old_run_section( section )
            if @module_name 
                set_unity_name( @module_name.gsub( /\//, '_' ) )
            end
        end

		def set_project_name(name)
			@project.name = name
		end
		
		def set_unity_name(name)
			set_attribute :unity_set, name
		end

		def set_gamebuild_dir(path)
			set_global_path :gamebuild_dir, path
		end

		def get_target_platform()
			platform = nil
			case @project.build_tags
			when tag('win32')
				return :win32
            when tag('linux')
                return :linux
            else
                raise "Unsupported target platform!"
			end
		end
		
		def get_host_platform()
			host_os = RbConfig::CONFIG['host_os'] 
            case host_os
            when 'mingw32'
			when 'mswin32'
				return :win32
            when 'linux-gnu'
                return :linux
			else
				raise 'Unsupported host platform ' + host_os.to_s + ' please contact jkoenen@motor3d.org for help!'
			end			
		end
		
		def set_compiler_option( language, option, value )
		end
		
		def add_copmiler_option( language, option, value )
		end
		
		def enable_cpp_exceptions
			add_global_attribute :c_define, 'MOTOR3D_EXCEPTIONS_ENABLED=1'
		end

		def add_c_define( *defines )
			add_attribute :c_define, *defines
		end
		
		def add_c_include_dir( *paths )
			add_path :c_include_dir, *paths
		end
		
		def add_lib_dir( *libdirs )
			add_global_path :c_lib_dir, *libdirs
		end
		
		def add_lib( *libs )
			add_global_attribute :c_lib, *libs
		end
		
		def add_dll( *dlls )
			add_global_path :dlls, *dlls
		end		

		def set_c_optimization(values)
			set_attribute :c_optimization, values
		end
		
		def add_shader_include_dir( *paths )
			add_path :shader_include_dir, *paths
		end
	end
end

# add some defines 
case @project.build_tags
when tag( 'debug' )
	add_global_attribute :c_define, 'SYS_BUILD_DEBUG'

when tag( 'release' )
	add_global_attribute :c_define, 'SYS_BUILD_RELEASE'

when tag( 'master' )
	add_global_attribute :c_define, 'SYS_BUILD_MASTER'
end

case get_target_platform()
when :win32
    add_global_attribute :c_define, "SYS_PLATFORM_WIN32"

when :linux
    add_global_attribute :c_define, "SYS_PLATFORM_LINUX"
end


