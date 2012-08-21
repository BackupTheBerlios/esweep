load ../esweep.so; # replace this line by something like package require ...
#load ./esweep.dll; # replace this line by something like package require ...

# set defaults
array set config {
	General,Samplerate 	48000
	General,Framesize 	2048
	General,Mode		twotone
	General,Channel		1
	General,Cal		cal
	Twotone,DF		10
	Twotone,Ratio		1
	Twotone,Sweep		off
	Twotone,Order		3
	Common,N		32
	Common,Spacing		lin
	Common,F1		200
	Common,F2		3150
	Common,Print		{off}
	Common,Level		{fixed}
}

proc usage {} {
	global config
	puts stderr "Usage:\nGeneral: \{tclsh\} imd.tcl -mode twotone/multitone/am \[$config(General,Mode)\] \
		-samplerate \[$config(General,Samplerate)\] -channel \[$config(General,Channel)\] -cal \[$config(General,Cal)\]"

	puts stderr "Mode twotone/am: -df frequency \[$config(Twotone,DF)\] -ratio \[$config(Twotone,Ratio)\] -order N \[$config(Twotone,Order)\]\n		
					-sweep off/single/both \[$config(Twotone,Sweep)\] -spacing lin/log \[$config(Common,Spacing)\] -n {# of tones} \[$config(Common,N)\]" 
					

	puts stderr "Mode multitone: -n {# of tones} \[$config(Common,N)\] -spacing lin/log \[$config(Common,Spacing)\]"
	puts stderr "All modes: -f1 \[$config(Common,F1)\] -f2 \[$config(Common,F2)\] -print off/spectrum \[$config(Common,Print)\] -level manual/{SPL} \[$config(Common,Level)\]"
}

# parse args

# get the mode
if {[catch {array set opts $argv}]} {usage; exit}
if {[info exists opts(-mode)]} {
	if {$opts(-mode) ne {twotone} && $opts(-mode) ne {multitone} && $opts(-mode) ne {am}} {
		puts stderr "Unknown mode $opts(-mode)"
		usage
		exit
	} else {
		set config(General,Mode) $opts(-mode)
	}
}
# remove mode from array
array unset opts -mode

foreach {opt val} [array get opts] {
	switch $opt {
		-samplerate {
			# simple validity check, if the samplerate is really supported
			# will be checked during soundcard opening
			if {$val <= 8000 || $val > 192000} {
				puts stderr {Unsupported samplerate.}
				exit
			}
			set config(General,Samplerate) $val
		}
		-channel {
			if {$val < 1 || $val > 2} {
				puts stderr {Unsupported channel.}
				exit
			}
			set config(General,Channel) $val
		}
		-cal {
                        # check the calibration option later
                        set config(General,Cal) $val
		}
		-df {
			if {$config(General,Mode) eq {twotone}} {
				if {$val <= 0} {
					puts stderr {Frequency difference must be >0}
					exit
				}
				set config(Twotone,DF) $val
			}
		}
		-ratio {
			if {$config(General,Mode) eq {twotone} || $config(General,Mode) eq {am}} {
				if {$val <= 0} {
					puts stderr {Level ratio must be >0}
					exit
				}
				set config(Twotone,Ratio) $val
			}
		}
		-sweep {
			if {$config(General,Mode) eq {twotone} || $config(General,Mode) eq {am}} {
				if {$val ne {both} && $val ne {single} && $val ne {off}} {
					puts stderr {-sweep must be \"both\", \"single\" or \"off\"}
					exit
				}
				set config(Twotone,Sweep) $val
			}
		}
		-order {
			if {$config(General,Mode) eq {twotone} || $config(General,Mode) eq {am}} {
				if {$val < 1} {
					puts stderr {-order must be greater 1}
					exit
				}
				set config(Twotone,Order) $val
			}
		}
		-n {
			if {$val <= 1} {
				puts stderr {# of tones must be >1}
				exit
			}
			set config(Common,N) $val
		}
		-f1 {
			if {$val <= 0} {
				puts stderr {Frequency f1 must be >0}
				exit
			}
			set config(Common,F1) $val
		}
		-f2 {
			if {$val <= 0} {
				puts stderr {Frequency f2 must be >0}
				exit
			}
			set config(Common,F2) $val
		}
		-spacing {
			if {$val ne {log} && $val ne {lin}} {
				puts stderr {Error: -spacing is neither "log" nor "lin".}
				exit
			}
			set config(Common,Spacing) $val
		}	
		-print {
			if {$val ne {spectrum} && $val ne {wave} && $val ne {off}} {
				puts stderr {Error: -print is neither "spectrum" nor "off".}
				exit
			}
			set config(Common,Print) $val
		}
		-level {
			if {$val ne {manual} && [catch {expr {int($val)}}]} {
				puts stderr {Error: -level must be "manual" or a SPL value.}
				exit
			}
			set config(Common,Level) $val
		}
		default {
			puts stderr "Unknown option $opt"
			usage
			exit
		}
	}
}

# additional validity checks
if {$config(General,Mode) eq {twotone}} {
	if {$config(Common,F2)+$config(Twotone,DF) >= $config(General,Samplerate)/2} {
		puts stderr {Error: Frequency out of range!}
		exit
	}
} else {
	if {$config(Common,F1) > $config(Common,F2)} {
		puts stderr {Error: F2 > F1!}
		exit
	}
	if {$config(Common,F2) >= $config(General,Samplerate)/2} {
		puts stderr {Error: Frequency out of range!}
		exit
	}
}

# parse calibration option
if {[catch {expr {int($config(General,Cal))}}]} {
        # NAN, interpret this as a filename
        if {[set fp [open $config(General,Cal) {r}]] ne {}} {
                # read 1st line
                gets $fp config(General,Cal)
                # is this a number?	
                if {[catch {expr {int($config(General,Cal))}}]} {
                        # no
                        puts stderr {Error: no valid calibration data found}
                        exit
                }
        } else {
                puts stderr "Error: calibration file $config(General,Cal) not found."
                exit
        }
} else {
        if {!($config(General,Cal) > 0)} {
                puts stderr {Error: calibration value not valid.}
                exit
        }
}
# calculate the step variable for linear or logarithmic spacing
proc getStep {from to n spacing} {
	if {$spacing eq {lin}} {
		set step "+ [expr {double($to - $from)/$n}]"
	} elseif {$spacing eq {log}} {
		set step "* [expr {pow(10, log10(double($to)/$from)/$n)}]"
	} else {
		puts stderr "Error: unknown spacing \"$config(Common,Spacing)\""
		exit
	}	
	return $step
}

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

proc record {au sig_out sig_in fftsize} {
	global config
	set delay [expr {int(0.5+1000*double($config(General,Framesize))/$config(General,Samplerate))/2}]
	# The audio subsystem needs 4 frames before playback really starts
	# so play 4 frames before actual recording
	set offset 0
	set old_offset $offset	
        while {1} {
		set frames 10 
	
                while {$frames > 0} {
                        esweep::audioIn -handle $au -signals $sig_in -offset $old_offset
                        set offset [esweep::audioOut -handle $au -signals $sig_out -offset $old_offset]
                        if {$offset > $old_offset} {incr frames -1}
                        if {$offset >= $fftsize} {set offset 0}
                        set old_offset $offset
                        after $delay
                }
                set frames [expr {5+$fftsize/$config(General,Framesize)}]
                while {$frames > 0} {
                        esweep::audioIn -handle $au -signals $sig_in -offset $old_offset
                        set offset [esweep::audioOut -handle $au -signals $sig_out -offset $old_offset]
                        if {$offset > $old_offset} {incr frames -1}
                        if {$offset >= $fftsize} {set offset 0}
                        set old_offset $offset
                        after $delay
                }
                # level control
                set in [lindex $sig_in [expr {$config(General,Channel)-1}]] 
                set out [lindex $sig_out [expr {$config(General,Channel)-1}]] 
                set lvl [expr {94+10*log10(([esweep::sqsum -obj $in]/$fftsize)/($config(General,Cal)**2))}]
                if {$config(Common,Level) eq {manual}} {
                        # simply print the level
                        puts [format "%8.8f dB SPL" $lvl]
                        if {abs($lvl-$config(Common,Level)) < 0.5} {return}
                } elseif {$config(Common,Level) eq {fixed}} {
                        # no level control, return
                        return
                } else {
                        # adjust the output signal until the level fits within a range of +/- 0.5 dB
                        if {abs($lvl-$config(Common,Level)) < 0.5} {return}
                        set max [esweep::max -obj $out]
                        set gain [expr {pow(10, ($config(Common,Level)-$lvl)/20)}]
                        if {$gain*$max > 0.9} {
                                puts stderr {Not enough headroom for automatic gain control.}
                                exit
                        } 
                        esweep::expr out * $gain
                } 
                        
        } 
        return
}

proc doTwotone {F1 F2} {
	global config
	set samplerate $config(General,Samplerate)
	set framesize $config(General,Framesize)
	set channel $config(General,Channel)
	set cal $config(General,Cal)
	set ratio $config(Twotone,Ratio)
	set level $config(Common,Level)
	set record $config(Common,Print)
	set mode $config(General,Mode)

	set DF [expr {abs($F2-$F1)}]
	# calculate the FFT size that the deviation from the desired frequency is below 5%
	if {$DF > $F1} {
		set dev [expr {0.05*$F1}]
		set fftsize [expr {int(0.5+pow(2, ceil(log(1.0*$samplerate/$dev)/log(2))))}]
	} else {	
		set dev [expr {0.05*$DF}]
		set fftsize [expr {int(0.5+pow(2, ceil(log(1.0*$samplerate/$dev)/log(2))))}]
	}
	set fftsize [expr {$fftsize < $framesize ? $framesize : $fftsize}]

	# open audio device and check if the configuration works
	if {[catch {set au [openAudioDevice audio:/dev/audio $samplerate $framesize]}]} {
		puts stderr {Error: needed framesize not supported by soundcard, adjust -df.}
		exit
	}

	# generate output signals
	set f1 [esweep::create -type wave -samplerate $samplerate -size $fftsize]
	set f2 [esweep::create -type wave -samplerate $samplerate -size $fftsize]

	set bin1 [esweep::generate sine -obj f1 -frequency $F1]
	set bin2 [esweep::generate sine -obj f2 -frequency $F2]

	if {$mode eq {twotone}} {
		# reduce level of f2
		esweep::expr f2 / $ratio
		# add them together
		esweep::expr f1 + $f2
	} elseif {$mode eq {am}} {
		# reduce level of f1
		esweep::expr f1 / $ratio
		# add DC to f1
		esweep::expr f1 + 1
		# multiply them together (AM)
		esweep::expr f1 * $f2
	} else {
		puts stderr {Error: Unknown mode.}
	}

	if {$level eq {manual} || $level eq {fixed}} {
	      # adjust level to -3dB peak
	      set lvl [expr {sqrt(2)*[esweep::max -obj $f1]}]
	} else {
	      # set level to -40 dB and let the record function control the volume itself
	      set lvl [expr {10*[esweep::max -obj $f1]}]
	}

	esweep::expr f1 / $lvl

	# generate input signals
	set in1 [esweep::create -type wave -samplerate $samplerate -size $fftsize]
	set in2 [esweep::create -type wave -samplerate $samplerate -size $fftsize]

	# record signal
	esweep::audioSync -handle $au
	record $au [list $f1 $f1] [list $in1 $in2] $fftsize 

	# close audio device
	esweep::audioSync -handle $au
	esweep::audioClose -handle $au

	if {$channel == 1} {set in $in1} else {set in $in2}

	# remove DC component
	esweep::expr in - [esweep::avg -obj $in]

	# calibration
	esweep::expr in / $cal

	if {$record eq {wave}} {
		esweep::toAscii -filename "IMD_${mode}_[expr {int($F1)}]_[expr {int($F2)}]_Wave.txt" -obj $in
	}
	# compute the IMD
	esweep::expr in * [expr {2*pow(10, 91.0/20)/$fftsize}]; 	# FFT scaling

	esweep::fft -inplace -obj in
	esweep::toPolar -obj in

	esweep::lg -obj in
	esweep::expr in * 20

	if {$record eq {spectrum}} {
		esweep::toAscii -filename "IMD_${mode}_[expr {int($F1)}]_[expr {int($F2)}]_Spectrum.txt" -obj $in
	}


	return [list $in $bin1 $bin2]
}

proc twotone {} {
	global config

	switch $config(Twotone,Sweep) {
		off {
			###################################################
			# Play two sines separated by $config(Twotone,DF) #
			###################################################

			lassign [doTwotone $config(Common,F1) $config(Common,F2)] in bin1 bin2

			# frequency and level of F1 
			lassign [esweep::index -obj $in -index $bin1] f1 l1
			# frequency and level of F2
			lassign [esweep::index -obj $in -index $bin2] f2 l2
			puts -nonewline stdout [format "%8.3f\t%8.3f\t%8.3f\t%8.3f\t" $f1 $l1 $f2 $l2]

			# get the levels of the IMD products
			set n 1
			set total 0
			while {$n < $config(Twotone,Order)} {
				set m 1
				# sum frequencies
				while {$m < $config(Twotone,Order)} {
					set b [expr {$m*$bin2 + $n*$bin1}]
					lassign [esweep::index -obj $in -index $b] f l
					set total [expr {$total+pow(10, $l/10.0)}]; # RMS
					#puts -nonewline stdout [format "%8.3f\t%8.3f\t" $f $l]
					incr m
				}	
				set m 1
				# difference frequencies
				while {$m < $config(Twotone,Order)} {
					set b [expr {$m*$bin2 + $n*$bin1}]
					lassign [esweep::index -obj $in -index $b] f l
					set total [expr {$total+pow(10, $l/10.0)}]; # RMS
					#puts -nonewline stdout [format "%8.3f\t%8.3f\t" $f $l]
					incr m
				}	
				incr n
			}
			set total [expr {10*log10($total)}]
			puts stdout [format "%8.3f" $total]
		}
		single {
			#############################################################
			# Perform an IMD sweep with one sine at fixed frequency F1. #
			# The other sine starts at F1+DF, and does N-1 steps until  #
			# it reaches F2. The spacing can be linear of logarithmic.  #
			#############################################################
			set step [getStep $config(Common,F1) $config(Common,F2) $config(Common,N) $config(Common,Spacing)]
			set F2 [expr $config(Common,F1) $step]
			for {set i 0} {$i < $config(Common,N)} {incr i} {
			lassign [doTwotone $config(Common,F1) $config(Common,F2)] in bin1 bin2
				lassign [doTwotone $config(Common,F1) $F2] in bin1 bin2

				# frequency and level of F1 
				lassign [esweep::index -obj $in -index $bin1] f1 l1
				# frequency and level of F2
				lassign [esweep::index -obj $in -index $bin2] f2 l2
				puts -nonewline stdout [format "%8.3f\t%8.3f\t%8.3f\t%8.3f\t" $f1 $l1 $f2 $l2]

				# get the levels of the IMD products
				set n 1
				set total 0
				while {$n < $config(Twotone,Order)} {
					set m 1
					# sum frequencies
					while {$m < $config(Twotone,Order)} {
						set b [expr {$m*$bin2 + $n*$bin1}]
						lassign [esweep::index -obj $in -index $b] f l
						set total [expr {$total+pow(10, $l/10.0)}]; # RMS
						#puts -nonewline stdout [format "%8.3f\t%8.3f\t" $f $l]
						incr m
					}	
					set m 1
					# difference frequencies
					while {$m < $config(Twotone,Order)} {
						set b [expr {$m*$bin2 + $n*$bin1}]
						lassign [esweep::index -obj $in -index $b] f l
						set total [expr {$total+pow(10, $l/10.0)}]; # RMS
						#puts -nonewline stdout [format "%8.3f\t%8.3f\t" $f $l]
						incr m
					}	
					incr n
				}
                                set total [expr {10*log10($total/$config(Twotone,Order))}]
				puts stdout [format "%8.3f" $total]
				set F2 [expr $F2 $step]

				after 500
			}
		}	
		both {
			########################################################
			# Sweep both tones from F1 to F2 and keep DF constant. #
			# The spacing can be linear of logarithmic.    	       #
			########################################################
			set step [getStep $config(Common,F1) $config(Common,F2) [expr {$config(Common,N)-1}] $config(Common,Spacing)]
			set F1 $config(Common,F1)
			for {set i 0} {$i < $config(Common,N)} {incr i} {
				lassign [doTwotone $F1 [expr {$F1+$config(Twotone,DF)}] in bin1 bin2

				# frequency and level of F1 
				lassign [esweep::index -obj $in -index $bin1] f1 l1
				# frequency and level of F2
				lassign [esweep::index -obj $in -index $bin2] f2 l2
				puts -nonewline stdout [format "%8.3f\t%8.3f\t%8.3f\t%8.3f\t" $f1 $l1 $f2 $l2]

				# get the levels of the IMD products
				set n 1
				set total 0
				while {$n < $config(Twotone,Order)} {
					set m 1
					# sum frequencies
					while {$m < $config(Twotone,Order)} {
						set b [expr {$m*$bin2 + $n*$bin1}]
						lassign [esweep::index -obj $in -index $b] f l
						set total [expr {$total+pow(10, $l/10.0)}]; # RMS
						#puts -nonewline stdout [format "%8.3f\t%8.3f\t" $f $l]
						incr m
					}	
					set m 1
					# difference frequencies
					while {$m < $config(Twotone,Order)} {
						set b [expr {$m*$bin2 + $n*$bin1}]
						lassign [esweep::index -obj $in -index $b] f l
						set total [expr {$total+pow(10, $l/10.0)}]; # RMS
						#puts -nonewline stdout [format "%8.3f\t%8.3f\t" $f $l]
						incr m
					}	
					incr n
				}
                                set total [expr {10*log10($total/$config(Twotone,Order))}]
				puts stdout [format "%8.3f" $total]
				set F1 [expr $F1 $step]

				after 500
			}
		}	


		default {
			puts stderr {Unknown sweep mode.}
			exit
		}
	}
}

proc multitone {} {
	global config
	set F1 $config(Common,F1)
	set samplerate $config(General,Samplerate)
	set framesize $config(General,Framesize)
	set channel $config(General,Channel)

	# calculate the FFT size that the deviation from the desired frequency is below 10%
	set dev [expr {0.05*$F1}]
	set fftsize [expr {int(0.5+pow(2, ceil(log(1.0*$samplerate/$dev)/log(2))))}]
	set fftsize [expr {$fftsize < $framesize ? $framesize : $fftsize}]

	set step [getStep $F1 $config(Common,F2) [expr {$config(Common,N)-1}] $config(Common,Spacing)]

	# create tones
	set out [esweep::create -type wave -samplerate $samplerate -size $fftsize] 
	set sine [esweep::create -type wave -samplerate $samplerate -size $fftsize] 

	expr {srand(1)}

	for {set i 0} {$i < $config(Common,N)} {incr i} {
		set phase [expr {rand()*acos(-1)}]
		lappend bins [esweep::generate sine -obj sine -frequency $F1 -phase $phase]
		esweep::expr out + $sine
		set F1 [expr $F1 $step]
	}

      if {$config(Common,Level) eq {manual} || $config(Common,Level) eq {fixed}} {
              # adjust level to -3dB peak
              set lvl [expr {sqrt(2)*[esweep::max -obj $out]}]
      } else {
              # set level to -40 dB and let the record function control the volume itself
              set lvl [expr {10.0*[esweep::max -obj $out]}]
      }
	esweep::expr out / $lvl


	# generate input signals
	set in1 [esweep::create -type wave -samplerate $samplerate -size $fftsize]
	set in2 [esweep::create -type wave -samplerate $samplerate -size $fftsize]

	# open audio device and check if the configuration works
	if {[catch {set au [openAudioDevice audio:0 $samplerate $framesize]}]} {
		puts stderr {Error: needed framesize not supported by soundcard, adjust -f1.}
		exit
	}

	# record signal
	esweep::audioSync -handle $au
	record $au [list $out $out] [list $in1 $in2] $fftsize 

	# close audio device
	esweep::audioSync -handle $au
	esweep::audioClose -handle $au

	if {$channel == 1} {set in $in1} else {set in $in2}
	
	# remove DC component
	esweep::expr in - [esweep::avg -obj $in]
	esweep::expr in / $config(General,Cal)

	if {$config(Common,Print) eq {wave}} {
		esweep::toAscii -filename "IMD_Multitone_$config(Common,F1)_$config(Common,F2)_$config(Common,N).txt" -obj $in
	}

	# compute the IMD
	esweep::expr in * [expr {2*pow(10, 94.0/20)/$fftsize}]; 

	esweep::fft -inplace -obj in
	esweep::toPolar -obj in

	# level of the wanted signal
	set lvl 0
	foreach bin $bins {
		lassign [esweep::index -obj $in -index $bin] f l
		set lvl [expr {$lvl+$l*$l}]
	}	

	# total level of the signal; factor 1/2 because of hermitian redundancy
	set dist [expr {[esweep::sqsum -obj $in]/2}]

	set tdn [expr {100*sqrt($dist/$lvl-1)}]
	puts stdout [format "%8.3f\t%8.3f\t%8.3f%%" [expr {10*log10($lvl)}] [expr {10*log10($dist-$lvl)}] $tdn]

	if {$config(Common,Print) eq {spectrum}} {
		esweep::lg -obj in
		esweep::expr in * 20 
		esweep::toAscii -filename "IMD_Multitone_$config(Common,F1)_$config(Common,F2)_$config(Common,N).txt" -obj $in
	}
}

switch $config(General,Mode) {
	twotone {
		twotone
	}
	am {
		# AM is a special version of twotone
		twotone
	}
	multitone {
		multitone
	}
	default {
		puts stderr {Error: unknown mode}
	}
}

