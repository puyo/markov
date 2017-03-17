require 'pp'
input = IO::readlines(ARGV[0]).map{|m| w = m.split[0] and w[0...-1]}.compact

File::open(ARGV[1], 'w') do |f|
	for n in input
		f.puts n
	end
end
