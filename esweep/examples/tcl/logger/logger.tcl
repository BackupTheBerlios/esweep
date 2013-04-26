#load ../../../libesweeptcl.so esweep; # replace this line by something like package require ...
load ./esweep.dll; # replace this line by something like package require ...

# set defaults
array set config {
	Audio,Samplerate 	8000
	Audio,Framesize 	2048
	Signal,Type		    sine
  Signal,Frequency  20
  General,Interval  1000
  General,Processing  rms
  General,OutputThreshold 5
}


proc openAudioDevice {device samplerate} {
	if {[catch {set au [esweep::audioOpen -device $device]}]} {
		puts stderr {Unable to open audio device.}
		exit 2
	}
	if {[catch {esweep::audioConfigure -handle $au -param samplerate -value $samplerate}]} {
		puts stderr {Unsupported samplerate.}
		exit 2
	}

	return $au
}

proc play {signal offset} {
  global config
  global au

  if {$offset < $config(Audio,Framesize)} {
    set offset [esweep::audioOut -handle $au -signals $signal -offset $offset]
  } else {
    set offset 0
  }
  # call myself after some time
  after 10 [list play $signal $offset]
  update
}

proc record {signal offset procFunc} {
  global config
  global au

  if {$offset < $config(Audio,Framesize)} {
    set offset [esweep::audioIn -handle $au -signals $signal -offset $offset]
    # call myself after some time
    after 10 [list record $signal $offset $procFunc]
    update
  } else {
    # record buffer full, process data
    $procFunc $signal
    # call myself after main interval
    after $config(General,Interval) [list record $signal 0 $procFunc]
  }
}


# logging function; stores data in memory until a threshold is reached, then writes to output file
set log [list]
proc logData {newData} {
  global log
  global config
  puts stdout "[expr {[clock milliseconds]-$config(General,StartTime)}] [join $newData]"
  #foreach token $newData {lappend log $token}
  #if {[llength $log] >= $config(General,OutputThreshold)} {
  #  puts stdout $log
  #  set log [list]
  #}
}

# PROCESSING FUNCTIONS
proc RMS {input} {
  set data [list]
  foreach sig $input {
    lappend data [expr {sqrt([esweep::sqsum -obj $sig]/[esweep::size -obj $sig])}]
  }
  logData $data
}

# INIT

# open audio device
set au [openAudioDevice audio:0 $config(Audio,Samplerate)]

# get framesize
if {[catch {set config(Audio,Framesize) [esweep::audioQuery -handle $au -param {framesize}]}]} {
  puts stderr {Cannot read framesize. Exiting.}
  exit 2
}

puts $config(Audio,Framesize)

set config(Audio,RecChannels) [esweep::audioQuery -handle $au -param {record_channels}]
set config(Audio,PlayChannels) [esweep::audioQuery -handle $au -param {play_channels}]
# get number of channels
if {[catch {set config(Audio,RecChannels) [esweep::audioQuery -handle $au -param {record_channels}]}]} {
  puts stderr {Cannot read number of record channels.}
  exit 2
}
if {[catch {set config(Audio,PlayChannels) [esweep::audioQuery -handle $au -param {play_channels}]}]} {
  puts stderr {Cannot read number of play channels.}
  exit 2
}

# setup processing function
switch $config(General,Processing) {
  rms {
    set procFunc RMS
  }
  default {
    puts stderr "Unsupported processing type \"$config(General,Processing)\""
    exit 1
  }
}

# create signal
set sig [esweep::create -type wave -samplerate $config(Audio,Samplerate) -size $config(Audio,Framesize)]

switch $config(Signal,Type) {
  sine {
    esweep::generate sine -obj sig -frequency $config(Signal,Frequency)
  }
  off {
    # do not create any signal
  }
  default {
    puts stderr "Unsupported signal type \"$config(Signal,Type)\""
    exit 1
  }
}

# setup output signal list
set output [list]
for {set i 0} {$i < $config(Audio,PlayChannels)} {incr i} {
  lappend output $sig
}

# setup input signal list
set input [list]
for {set i 0} {$i < $config(Audio,RecChannels)} {incr i} {
  lappend input [esweep::create -type wave -samplerate $config(Audio,Samplerate) -size $config(Audio,Framesize)]
}

# init playback
play $output 0

# init record
after $config(General,Interval) [list record $input 0 $procFunc]

set config(General,StartTime) [clock milliseconds]
set forever 0
vwait forever
