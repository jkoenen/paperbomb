# This file implements the Lace::Context class.

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

module Lace
	# Contexts hold attributes to be used by compilers.
	class Context
		def initialize(project, parent)
			project.register_context(self)
			@parent = parent
			@imports = []
			@auto_tags = []
			@attributes = {}
		end
		
		def add_import(import)
			@imports << import
		end
		
		def resolve_imports
			@imports = @imports.map do |import|
				next if import.is_a?(Project::Import) and !import.module
				import.is_a?(Project::Import) ? import.module.exports : import
			end.flatten
		end
		
		def add_attribute(id, *values)
			attribute = (@attributes[id] ||= [])
			raise "Trying to add values to scalar attribute #{id.inspect}" unless attribute.is_a?(Array)
			attribute.concat(values)
		end
		
		def set_attribute(id, value)
			if @attributes.include?(id)
				raise "Trying to set attribute #{id.inspect} but it already has a value"
			end
			@attributes[id] = value
		end
		
		# this returns a list of uniq values of the attribute named by +id+ found in
		# this context and in all parent contexts
		def get_attribute_set(id)
			get_attribute_list(id).uniq
		end
		
		# this returns a list of values of the attribute named by +id+ found in
		# this context and in all parent contexts
		def get_attribute_list(id)
			values = @parent ? @parent.get_attribute_list(id) : []
			@imports.each do |import|
				next if !import
				values.concat(import.get_attribute_list(id))
			end
			my_values = @attributes[id]
			if my_values
				if my_values.is_a?(Array)
					values.concat(my_values)
				else
					values << my_values
				end
			end
			return values
		end
		
		# this returns the first value found for the attribute named by +id+ in this
		# context or any of its parent contexts
		def get_attribute_value(id)
			attribute = @attributes[id]
			return attribute.is_a?(Array) ? attribute.last : attribute if attribute
			@imports.each do |import|
				next if !import
				value = import.get_attribute_value(id)
				return value if value
			end
			return @parent ? @parent.get_attribute_value(id) : nil
		end
		
		# this returns a list of value defined for the attribute named by +id+ defined
		# in just this context
		def get_local_attributes(id)
			@attributes[id] || []
		end
		
		def add_auto_tags(tags, filter)
			@auto_tags << [[tags].flatten, filter]
		end
		
		def evaluate_auto_tags(tags)
			@auto_tags.each do |new_tags, filter|
				if !filter || filter.matches?(tags)
					new_tags.each {|t| tags << t }
				end
			end
			return @parent ? @parent.evaluate_auto_tags(tags) : tags
		end
	end
end

