# Copyright (c) 2009 keen games
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

require 'lace/inputfile'
require 'lace/compilerbase'
require 'lace/lacemodule'
require 'lace/context'
require 'lace/compilerbase'
require 'lace/tag'
require 'ostruct'
require 'set'

module Lace
	class LaceError < Exception
	end

	class Project
		include PathMixin
		Import = Struct.new :name, :module, :imported_from, :weak_import
		
		attr_reader :global_context, :files, :compiler_set, :build_tags, :globals, :filename
		attr_reader :build_path, :verbose, :contexts, :modules, :parent_project, :sub_projects
		attr_reader :post_build_steps, :path, :used_files
		attr_accessor :name, :parent_context, :output_filter
		
		def initialize(filename, build_sub_path, build_tags, parent_project = nil)
			@filename = filename
			@path = parent_project ? parent_project.build_path : filename.dirname
			if @build_sub_path.is_a?(Pathname)
				@build_path = build_sub_path
			else
				@build_sub_path = build_sub_path
				@build_path = @path + build_sub_path
			end
			@parent_project = parent_project
			@sub_projects = []
			@modules = []
			@imports = {}
			@module_options = {}
			@import_paths = []
			@contexts = []
			@files = []
			@file_mapping = {}
			@global_context = Context.new(self, nil)
			@compiler_set = CompilerSet.new
			@compiler_white_list = []
			@build_tags = build_tags
			@globals = OpenStruct.new
			@name = 'unnamed'
			@module_aliases = {}
			@post_build_steps = []
			@used_files = Set.new
		end
		
		def read_module(filename, options = nil, section = nil, module_name = nil)
			add_used_file(filename)
			path = to_path(filename)
			current_dir = Dir.getwd
			Dir.chdir(path.dirname)
			src = path.readlines
			src = src.map do |line|
				case line
				when /^\s*!\s*(\S+)((\s+(\S+))*)\s*$/
					tags = ($2 || '').strip
					"add_file('#{$1}', '#{tags}')\n"
				else
					line
				end
			end
			
			mod = LaceModule.new(self, path.dirname, options || {}, module_name)
			@modules << mod
			begin
				mod.instance_eval(src.join, filename.to_s)
			rescue LaceError => e
				Helpers.trace_error(e.backtrace, "Error: %s", e.message)
				exit 1
			end
			
			if section
				if mod.sections[section]					
					mod.run_section( section )
				else
					Helpers.trace("Error: Unable to find section '%s' in module '%s'", section, get_module_display_name(path))
					return nil
				end
			else
				mod.run_section( 'main' )				
			end
			
			Dir.chdir(current_dir)
			
			return mod
		end
		
		def define_subproject(name, params, parent_context, block)
			build_tags = params[:build_tags] || @build_tags
			project = Project.new(@path, name, build_tags, self)
			project.parent_context = parent_context
			project.name = name
			@sub_projects << project
			@import_paths.each {|p| project.add_import_path(p) }
			@module_aliases.each {|n, o| project.add_module_alias(n, o) }
			mod = LaceModule.new(project, to_path(Dir.getwd), {}, nil)	# PT: is nil correct here ?
			project.modules << mod
			mod.instance_eval(&block)
			project.resolve_imports
			filter_tags = params[:output_filter_tags]
			if filter_tags
				project.output_filter = proc do |files|
					files.select {|file| filter_tags.matches?(file.tags) }
				end
			end
		end
		
		def add_module_alias(new_name,old_name)
			@module_aliases[new_name] = old_name
		end

		def add_weak_module_alias(new_name, old_name)
			@module_aliases[new_name] = @module_aliases[new_name] || old_name
		end

		def add_import(module_name, importing_module,weak)
			import = @imports[module_name] || Import.new(module_name, nil, [],weak)
			import.imported_from << importing_module if importing_module 
			@imports[module_name] = import
			return import
		end
		
		def get_imported_modules(base_module)
			@imports.values.select { |x| x.imported_from.include?( base_module ) }.map { |x| x.module }
		end
		
		def add_module_options(module_name, options)
			import = @imports[module_name]
			import.module = nil if import
			opt_hash = (@module_options[module_name] ||= {})
			options.each do |key, value|
				if opt_hash.include?(key) && opt_hash[key] != value
					raise "Changing option #{key} of module #{module_name} from #{opt_hash[key].inspect} to #{value.inspect}"
				end
				opt_hash[key] = value
			end
		end
		
		def add_import_path(path)
			@import_paths << to_path(path)
		end
		
		def register_context(context)
			@contexts << context
		end
		
		def get_module_display_name(module_path)
			return @name if module_path == @path
			
			shortest_path = nil
			@import_paths.each do |path|	
				if module_path.to_s.index( path.to_s ) == 0
					relative_module_path = module_path.to_s[ path.to_s.length + 1, module_path.to_s.length ]
					
					if find_module( relative_module_path )
						if shortest_path == nil || relative_module_path.length < shortest_path.length
							shortest_path = relative_module_path
						end
					end					
				end	
			end
			return shortest_path
		end

		def get_module_import_path(module_path)
			@import_paths.each do |path|	
				if ( module_path.to_s + '/' ).index( path.to_s + '/' ) == 0
					return path.to_s
				end	
			end
			return module_path.to_s
		end

		def resolve_module_alias( name )
			@module_aliases[ name ] || name
		end
		
		def find_module(name, default_filename = 'module.lace',error_on_duplicates = false)
			resolvedName = resolve_module_alias( name )
			
			if resolvedName == nil 
				resolvedName = "local"
			end
			
			found_filenames = []
			@import_paths.each do |path|
				module_path = path + resolvedName
				if module_path.exist?
					# first try: absolute module path given
					filename = module_path		
					# second try: last part of module path:
					module_filename = resolvedName.to_s.split( '/' ).last.to_s + ".lace"
					filename = module_path  + ( module_filename ) unless filename.file? 
					# third try: default_filename
					filename = module_path + default_filename unless filename.file? 
					found_filenames << filename if filename.file?
				end
			end
			
			found_filenames.uniq!
			
			if error_on_duplicates and found_filenames.length > 1
				raise "\n\nDuplicate module path found for module '" + name + "': \n"  + found_filenames.join( "\n" ) + "\n\n"
			end
		
			if found_filenames.length > 0 
				return found_filenames[ 0 ]
			else
				return nil
			end
		end
		
		def resolve_imports
			while @imports.any? {|n, i| !i.module}
				@imports.to_a.each do |name, import|
					next if import.module
					name, section = name.split('#', 2)
					full_path = find_module(name,'module.lace',true)
					if full_path
						import.module = read_module(full_path, @module_options[name], section, name)
						unless import.module
							STDERR.printf "Failed to load module '%s'.\nImported from:\n", name
							import.imported_from.each do |mod|
								STDERR.printf "  %s\n", mod.path
							end
							exit 1
						end
					else 
						if !import.weak_import
							STDERR.printf "Module '%s' could not be found.\nImported from:\n", name
							import.imported_from.each do |mod|
								STDERR.printf "  %s\n", mod.path
							end
							STDERR.printf "Search directories:\n"
							@import_paths.each do |path|
								STDERR.printf "  %s\n", path.to_s
							end
							exit 1
						end
					end
				end
			end
			
			@contexts.each {|context| context.resolve_imports }
		end
		
		def add_file(input_file)
			file = @file_mapping[input_file.path]
			if file
				file.tags = file.tags + input_file.tags
			else
				@files << input_file
				@file_mapping[input_file.path] = input_file
			end
		end
		
		def find_import_cycles
			visited_modules = {}
			cycles = Set.new
			
			visit_module = proc do |mod, path|
				if visited_modules[mod] == :visited
					cycles << mod
					next []
				end
				
				unless visited_modules[mod]
					visited_modules[mod] = :visited
					
					imported_modules = get_imported_modules(mod)
					imports = Set.new(imported_modules.map {|m| m.path })
					
					for imported_module in imported_modules
						if imported_module.path == mod.path
							imports.merge(visit_module[imported_module, path])
						else
							imports.merge(visit_module[imported_module, path + [mod]])
						end
					end
					
					visited_modules[mod] = imports
				end
				
				imports = visited_modules[mod]
				
				for m in path
					if imports.include?(m.path)
						cycles << m
					end
				end
				
				imports
			end
			
			for mod in @modules
				visit_module[mod, []]
			end
			
			unless cycles.empty?
				puts 'Cycles detected:'

				find_cycle_path = proc do |mod, path, target|
					for imported_mod in get_imported_modules(mod)
						if imported_mod.path == target
							throw :found, path + [imported_mod]
						end
						find_cycle_path[imported_mod, path + [imported_mod], target]
					end
				end
				
				for mod in cycles
					puts catch(:found) { find_cycle_path[mod, [mod], mod.path] }.map {|p| p.path.basename }.join(' -> ')
				end
			
				exit 1
			end
		end

		def self.load(filename, build_tags = [], build_dir = nil)
			$log.info("Loading project file #{filename} with build=#{build_tags.join('/')}") if $log
			Helpers.trace "Loading project file '%s' with build='%s'", filename, build_tags.join('/')
			filename = Pathname.new(filename).expand_path
			path = filename.dirname
			if build_dir
				build_dir = Pathname.new(build_dir).expand_path + build_tags.join('/')
			else
				build_dir = 'build/' + build_tags.join('/')
			end
			project = self.new(filename, build_dir, build_tags)
			project.add_import_path(path)
			project.read_module(filename)
			project.resolve_imports
			
			# check for cycles in the dependency graph:			
			project.find_import_cycles

			return project
		end
		
		def path=(path)
			if @parent_project
				puts "error: @project.path= is not allowed for sub-projects"
				exit 1
			end
			unless @build_sub_path
				puts 'error: build path clash (lace command line vs. @project.path='
				exit 1
			end
			@path = to_path(path)
			@build_path = @path + @build_sub_path
		end
		
		def add_compilertag_to_whitelist( tag )		
			@compiler_white_list << tag
		end
		
		def add_compiler( compiler )
			if not @compiler_white_list.empty? and not compiler.input_pattern.matches?( @compiler_white_list )
				return
			end			
			@compiler_set << compiler
		end
		
		def set_project_output_root_path(path)
			puts "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
			puts "! @project.set_project_output_root_path is deprecated !"
			puts "! and will be removed in the future.                  !"
			puts "! Please use @project.path = ... instead.             !"
			puts "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
			
			if path =~ /^(.+)\/build$/
				self.path = $1
			else
				puts "Only paths ending in '/build' are supported by @project.set_project_output_root_path"
				exit 1
			end
		end
		
		def add_used_file(filename)
			@used_files << File.expand_path(filename)
		end

	end
end

