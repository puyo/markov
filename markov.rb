#!/usr/bin/env ruby

class Array
  # A random element.
  def random
    return self[rand(self.size)]
  end
end

class String
  # This string, indented by indent and wrapped to width.
  def wrap(indent, width)
    result = ""
    words = split(/\s+/)
    while words.size > 0
      maxlen = width - indent
      line = []
      linelen = 0
      words.each do |w|
        if linelen == 0 and w.size > maxlen
          line << w
          break
        elsif linelen + w.size <= maxlen
          line << w; linelen += w.size + 1
        else
          break
        end
      end
      words = words[line.size, words.size]
      result += " "*indent + line.join(' ') + "\n"
    end
    return result
  end
end

# Markov Name Generator. Generates names using Markov chains.
class MarkovNameGenerator

  # Array of strings used while creating the last name.
  attr :progress

  # Array of all strings given as input.
  attr :input_set

  # Create a Markov name generator with specified randomness and
  # n-gram size.
  def initialize(randomness, ngram_size)
    @chains = []
    @input_set = []
    @randomness, @ngram_size = randomness, ngram_size
  end

  # Read a list of names from a file.
  # Argument can be a filename (string) or an open file.
  def read(file)
    begin
      if file.class == String
        f = File::open(file)
      else
        f = file
      end
      f.each_line do |line|
        input(line.strip)
      end
    ensure
      f.close
    end
  end

  # Input a single string into the generator.
  def input(string)
    string.downcase!
    @input_set << string
    (string.size - @ngram_size).times do |i|
      key = string[i, @ngram_size]
      remainder = string[i + @ngram_size, string.size]
      i.downto(0) do |j|
        @chains[j] = {} unless @chains[j]
        @chains[j][key] = [] unless @chains[j][key]
        @chains[j][key] << remainder
      end
    end
  end

  # A randomly generated name.
  def name
    # Use a random name to start.
    name = @input_set.random
    @progress = []
    pos = 0
    loop do
      chain = @chains[pos] or break
      key = name[pos, @ngram_size] or break
      remainders = chain[key] or break
      if rand(100) < @randomness
        remainder = remainders.random
        name = name[0, pos + @ngram_size]
        name += remainder
      end
      @progress << name
      pos += 1
    end
    return name.capitalize
  end
end

def usage
  puts "Usage:"
  puts "\t#{File::basename($0)} [-N] [-s S] [-r R] [-q] [-h] files"
  puts "\t#{File::basename($0)} [-N] [-s S] [-r R] [-q] [-h] < file"
  puts "\tcat files | #{File::basename($0)} [-N] [-s S] [-r R] [-q] [-h]"
  puts
  puts "\t-N\tProduce N names."
  puts "\t-s S\tSpecify n-gram size."
  puts "\t-r R\tSpecify randomness."
  puts "\t-q\tQuiet mode. Print one name per line."
  puts "\t-h\tDisplay this message."
  puts "\tfile(s)\tFiles containing input strings."
  exit
end

# Defaults.
iterations = 10
ngram_size = 2
randomness = 100
quiet = false

# Read arguments.
loop do
  case ARGV[0]
  when /^-(\d+)$/
    ARGV.shift
    iterations = $1.to_i
  when /^-s$/
    ARGV.shift
    ngram_size = ARGV.shift.to_i
  when /^-r$/
    ARGV.shift
    randomness = ARGV.shift.to_i
  when /^-q$/
    ARGV.shift
    quiet = true
  when /^-h$/
    usage
  else
    break
  end
end

exit unless iterations > 0

m = MarkovNameGenerator::new(randomness, ngram_size)
m.read(ARGF)

if not quiet
  title = "Names with N-Gram Size #{ngram_size}"
  puts title
  puts "-"*title.size
  orig_count = 0
end

iterations.times do
  n = m.name
  if quiet
    puts n
  else
    if m.input_set.include?(n.downcase)
      n = "  " + n
    else
      orig_count += 1
      n = "* " + n
    end
    origin = '[' + m.progress.join(', ') + ']'
    puts n + "\n" + origin.wrap(4, 60)
  end
end

if not quiet
  printf "(* = not in input set: %d%% originality)\n", (100*orig_count)/iterations
end
