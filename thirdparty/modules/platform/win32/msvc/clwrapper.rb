require 'open3'

objfile = ARGV.shift

error_found = false

cmd = ARGV.map {|a| / / =~ a ? '"'+a+'"' : a}.join(' ')

def parse_lines(io, dependencies)
	line = io.gets
	case line
	when /^Note: including file:\s+(.*)$/
		dependencies << $1
	when /^\w+\.c(pp)?$/
	when /error/
		puts line
		error_found = true
	else
		puts line
	end
end

ENV.delete('VS_UNICODE_OUTPUT')

dependencies = []

p = Open3.popen3(cmd) do |io_in, io_out, io_err|
	parse_lines(io_out, dependencies) until io_out.eof?
	puts io_err.gets until io_err.eof?
end

unless objfile == '-'
	File.open(objfile.sub(/\.[^\.]+$/, '.d'), 'w') do |file|
		file.printf "%s|%s", objfile, dependencies.join(';')
	end
end

