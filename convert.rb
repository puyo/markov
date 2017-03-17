
input = File::readlines(ARGV[0]).join(" ").strip.split(/[ \|]/).collect {|e| e if e != ''}.compact

File::open(ARGV[1], 'w') do |f|
	for n in input
		f.puts n
	end
end
