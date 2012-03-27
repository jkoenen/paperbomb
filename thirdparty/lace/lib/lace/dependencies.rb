require 'set'
require 'thread'

module Lace
	class Dependencies
		attr_reader :inputs, :outputs
		
		READ_MUTEX = Mutex.new # possible workaround for a ruby bug
		
		def initialize
			@inputs = []
			@outputs = []
		end
		
		def add_input(filename)
			@inputs << filename
		end
		
		def add_output(filename)
			@outputs << filename
		end
		
		def newer?
			@outputs.empty? || @inputs.empty? || Helpers.newer?(@inputs, @outputs)
		end
		
		def write_lace_dependencies(filename)
			inputs, outputs = [@inputs, @outputs].map do |paths|
				result = Set.new
				paths.each do |path|
					result << File.expand_path(path.to_s)
				end
				result.to_a
			end
			written = false
			until written == true
				begin
					File.open(filename, 'w') {|f| f.write outputs.join(';') + '|' + inputs.join(';') }
					written = true
				rescue Errno::EBADF
					# do nothing. This happens occasionally, we don't know why.
				end
			end

		end
		
		def self.load_lace_dependencies(filename)
			file = nil
			READ_MUTEX.synchronize do
				until file
					begin
						file = File.read(filename)
					rescue Errno::ENOENT
						# do nothing. This happens occasionally, we don't know why.
					end
				end
			end
			o, i = file.chomp.split('|')
			dependencies = Dependencies.new
			o.split(';').each {|output| dependencies.add_output(output) } if o
			i.split(';').each {|input| dependencies.add_input(input) } if i
			return dependencies
		end
		
		# parses a makefile format dependencies file and returns a Dependencies instance
		def self.load_make_dependencies(filename)
			dependencies = self.new
			file = nil
			READ_MUTEX.synchronize do
				file = File.read(filename)
			end
			file.gsub(/\\\n/, ' ').scan(/("[^"]+"|[^":]+):(.*)/) do |o, i|
				i = i.scan(/("[^"]+"|(\\ |\S)+)/).map {|d| d.first.gsub(/\\ /, ' ') }
				o.strip!
				o = o[1...-1] if o =~ /^".*"$/
				i = i.map do |n|
					n =~ /^".*"$/ ? n[1...-1] : n
				end
				i.each {|input| dependencies.add_input(input) }
				dependencies.add_output(o)
			end
			return dependencies
		end
	end
end

