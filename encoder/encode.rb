#!/usr/bin/env ruby

$LOAD_PATH << File.dirname( __FILE__ )

require 'optparse'
require 'set'

require 'lib/Print.rb'
require 'lib/CallGraphGenerator.rb'
require 'lib/Instruction.rb'
require 'lib/Function.rb'
require 'lib/FunctionGraphRenderer.rb'
require 'lib/InstructionFilter.rb'
require 'lib/ExploitPotentialEvaluator.rb'
require 'lib/GadgetFinder.rb'
require 'lib/ObjdumpReader.rb'
require 'lib/Cache.rb'
require 'lib/Results.rb'
require 'lib/Shellcode.rb'
require 'lib/GraphSearch.rb'

$options = {}
optparse = OptionParser.new do |opts|
  opts.banner = "Usage: #{__FILE__} [options] ADDRS_FILE"

  $options[:binary] = nil
  opts.on( '-b', '--binary FILE', 'ELF binary' ) do |file|
    $options[:binary] = file
  end

  $options[:output] = nil
  opts.on( '-o', '--output FILE', 'Output file' ) do |file|
    $options[:output] = file
  end
end

begin
  optparse.parse!
rescue OptionParser::InvalidOption
  STDERR.puts "[!] Invalid options"
  STDERR.puts optparse
  exit 1
end

INPUT_PATH = ARGV[0]

if INPUT_PATH.nil? || !File.exists?( INPUT_PATH )
  STDERR.puts "[!] Please provide a path to the input file."
  STDERR.puts optparse
  exit 1
end

if $options[:binary].nil? || !File.exists?( $options[:binary] )
  STDERR.puts "[!] Please provide a path to the binary."
  STDERR.puts optparse
  exit 1
end

if !ObjdumpReader.x86ELF?( $options[:binary] )
  STDERR.puts "[!] Binary should be a 32-bit ELF."
  STDERR.puts optparse
  exit 1
end

if $options[:output].nil?
  STDERR.puts "[!] Please provide an output path."
  STDERR.puts optparse
  exit 1
end

gen = CallGraphGenerator.new( $options[:binary] )
functions = gen.generate

input = File.open(INPUT_PATH, "r")
output = File.open($options[:output], "w")

last_was_invalid = false

input.each_line do |line|
  address = line.to_i(16)

  in_func = nil
  functions.each do |func|
    if func.start_addr <= address && address < func.end_addr
      in_func = func
      break
    end
  end

  in_func_instr_num = nil
  if in_func.nil?
    unless last_was_invalid
      output.write("\xFF\xFF\xFF\xFF\x00\x00\x00\x00")
      last_was_invalid = true
    end
  else
    last_was_invalid = false
    0.upto(in_func.disassembly.length - 1) do |instr_num|
      instr = in_func.disassembly[instr_num]

      if instr.address == address
        in_func_instr_num = instr_num
        break
      end
    end

    if in_func_instr_num.nil?
      puts "Mis-aligned instruction."
      exit 1
    end

    output.write(
      [functions.find_index(in_func), in_func_instr_num].pack("L<L<")
    )
  end

end

input.close
output.close
