package require Tk 8.5
package require Ttk 8.5

source ../TkEsweepXYPlot/TkEsweepXYPlot.tcl

#load ../../../libesweeptcl.so esweep; # replace this line by something like package require ...
load ./libesweeptcl.dll esweep; # replace this line by something like package require ...

# set defaults
array set config {
	General,Samplerate 	8000
	General,Framesize 	2048
	General,Channel		1
  Burst,Frequency   1000
  Burst,Duration    1000
  Burst,Interval    2000
}

set signal {}
set updBurstID {}
# play flag: -1: inactive, 0: paused, 1: active
set play -1
set au {}

proc openAudioDevice {device samplerate framesize} {
	if {[catch {set au [esweep::audioOpen -device $device]}]} {
		puts stderr {Unable to open audio device. Exiting.}
		exit
	}
	if {[catch {esweep::audioConfigure -handle $au -param samplerate -value $samplerate}]} {
		puts stderr {Unsupported samplerate.}
		exit
	}

	if {[catch {esweep::audioConfigure -handle $au -param framesize -value $framesize}]} {
		return -code error
	}
	return $au
}

proc playSignal {size} {
  global config
  global play
  global signal
  global au

  # call myself again; UI must take care that interval is always greater than duration of signal
  if {$play == 1} {
    set offset 0
    while {$offset < $size} {
      set offset [esweep::audioOut -handle $au -signals [list $signal] -offset $offset]
      # wait a moment
      after 10
      update
    }
    after $config(Burst,Interval) [info level 0]
  }
  update
}

# avoid manual spinbox manipulation
proc invalidateSpinbox {} {
	return 0
}

proc UI {} {
  global config

  wm iconify .

  # build burst view window
  set burst [canvas .burst -bg white -width 800 -height 600]
 	::TkEsweepXYPlot::init $burst Burst
	set ::TkEsweepXYPlot::plots(Burst,Config,Title) {Burst}
	set ::TkEsweepXYPlot::plots(Burst,Config,Cursors,1) {off}
	set ::TkEsweepXYPlot::plots(Burst,Config,Cursors,2) {off}

	set ::TkEsweepXYPlot::plots(Burst,Config,X,Label) {t [ms]}
	set ::TkEsweepXYPlot::plots(Burst,Config,X,Precision) 5
	set ::TkEsweepXYPlot::plots(Burst,Config,X,Autoscale) {on}
	set ::TkEsweepXYPlot::plots(Burst,Config,X,Bounds) [list 0 10000]
	set ::TkEsweepXYPlot::plots(Burst,Config,X,Log) {no}

	set ::TkEsweepXYPlot::plots(Burst,Config,Y1,Label) {Level}
	set ::TkEsweepXYPlot::plots(Burst,Config,Y1,Precision) 3
	set ::TkEsweepXYPlot::plots(Burst,Config,Y1,Autoscale) {on}
	set ::TkEsweepXYPlot::plots(Burst,Config,Y1,Bounds) [list -1 1]
	set ::TkEsweepXYPlot::plots(Burst,Config,Y1,Log) {no}

  # build configuration frame
  set c [frame .config]

	label $c.f_l -text {Frequency [Hz]: } -width 20 -anchor w
	::ttk::spinbox $c.f -textvariable config(Burst,Frequency) \
		-width 10 -from 10 -to [expr {$config(General,Samplerate)/2-1}] -increment 1 -validatecommand {invalidateSpinbox} -validate all \
    -command updateFrequency

	label $c.d_l -text {Duration [ms]: } -width 20 -anchor w
	::ttk::spinbox $c.d -textvariable config(Burst,Duration) \
		-width 10 -from 10 -to 2000 -increment 10 -validatecommand {invalidateSpinbox} -validate all \
    -command updateTiming

	label $c.i_l -text {Interval [ms]: } -width 20 -anchor w
	::ttk::spinbox $c.i -textvariable config(Burst,Interval) \
		-width 10 -from 20 -to 4000 -increment 10 -validatecommand {invalidateSpinbox} -validate all \
    -command updateTiming

  button $c.play -text {Play} -command updatePlay

  grid $c.f_l $c.f
  grid $c.d_l $c.d
  grid $c.i_l $c.i
  grid $c.play -columnspan 2

  # pack and draw the windows in the background
  pack $burst -side left -expand yes -fill both
  pack $c -side left

 	::TkEsweepXYPlot::plot Burst

  # deiconify root window
  wm deiconify .
}

proc updateFrequency {} {
  global config
  global updBurstID
  if {$config(Burst,Frequency) < 100} {
    .config.f configure -increment 1
  } elseif {$config(Burst,Frequency) < 1000} {
    .config.f configure -increment 10
  } else {
    .config.f configure -increment 100
  }

  catch {after cancel updBurstID}
  set updBurstID [after 200 updateBurst]
}

proc updateTiming {} {
  global config
  global updBurstID

  # let the interval be always greater than the duration
  .config.i configure -from [expr {$config(Burst,Duration)+10}]
  if {$config(Burst,Interval) < $config(Burst,Duration)+10} {set config(Burst,Interval) [expr {$config(Burst,Duration)+10}]}

  catch {after cancel updBurstID}
  set updBurstID [after 200 updateBurst]
}

proc updateBurst {} {
  global config
  global signal
  global play

  # disable playback when active
  if {$play == 1} {set play 0}

  # build burst signal
  set size [expr {int(0.5+$config(Burst,Duration)*$config(General,Samplerate)/1000.0)}]
  set signal [esweep::create -type wave -samplerate $config(General,Samplerate) -size $size]

  # create the sine with the necessary frequency resolution
  set sine_size [expr {$config(General,Samplerate)/[.config.f cget -increment]}]
  set sine_size [expr {$sine_size > $size ? $sine_size : $size}]
  set sine [esweep::create -type wave -samplerate $config(General,Samplerate) -size $sine_size]

  esweep::generate sine -obj sine -frequency $config(Burst,Frequency)
  esweep::expr signal + $sine
  esweep::window -obj signal -rightwin {hann} -leftwin {hann} -rightwidth 10 -leftwidth 10

	if {[::TkEsweepXYPlot::exists Burst 0]} {
		::TkEsweepXYPlot::configTrace Burst 0 -trace $signal
	} else {
		::TkEsweepXYPlot::addTrace Burst 0 Burst $signal
	}
  ::TkEsweepXYPlot::plot Burst

  update

  # enable playback when we disabled it
  if {$play == 0} {
    set play 1
    playSignal [esweep::size -obj $signal]
  }
}

proc updatePlay {} {
  global play
  global signal
  if {$play == -1} {
    .config.play configure -text {Mute}
    set play 1
    playSignal [esweep::size -obj $signal]
  } elseif {$play == 1} {
    .config.play configure -text {Play}
    set play -1
  }
}

# init
UI
set au [openAudioDevice audio:0 $config(General,Samplerate) $config(General,Framesize)]
updateBurst

