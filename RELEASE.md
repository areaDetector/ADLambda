ADDriver Releases

2017-03-01 R1-0
 * This version has been built against 
   * EPICS 3.14.12.5, synApps_5_8 (some packages to work with areadetector 2.0:calc-3-4-2. busy1-6-1, sscan-2-10-1, autosave-5-6-1, asyn-4-26) and areaDetector2-0, ADBinaries-2-2, ADCore-2-2
and
   * EPICS 3.15.5, synApps_5_8 (some packages to work with areadetector 2.5:calc-3-6-1. busy1-6-1, sscan-2-10-2, autosave-5-7-1, asyn-4-30) and areaDetector2-5 & ADSupport-1-0, ADCore-2-5
   * With 3.15.5 Was also built with EPICS-CPP-4.6.0 to allow NDPluginPva to allow some experimentation with pvAccess.

 * This version has most features of the detector working.  We have collected data at up to 2000 fps saving this data to IMM files using a custom PipeWriter File Plugin which passes data to 
an MPI program which compresses and saves the data to IMM.  At slower rates, other file plugins have been used successfully.

 * Much of the detector configuration is handled via configuration files.  Setting exposure time and switching between 12 and 24 bit acquisition are enabled.

 * Switching to external trigger modes is also enabled.

 * Switching to all data types except Float types is supported.

2019-01-21 R1-2
 * This version has been built against:
   * EPICS base 7.0.2
   * synApps 6_0
   * areaDetector 3-4, ADCore 3-4, ADSupport 1-6
   * was also built against base-7.0.1, synApps 5-8 & areaDetector 3-2 ADCore 3-2 ADSupport 1-4& some synApps modules updated to work with This AD version.
 * Updated to get rid of some firmware/vendor lib issues with the image counter overflowing.  This is a 24 bit counter which rolls over somewhat frequently.  Originally this would crash the driver.  firmware & driver fixed the crash but it is still a 24 bit counter.  This still rolls over.
 * Take out  the custom PipeWriter plugin.
 * Now using the Scatter/Gather plugin along with a modified IMM file plugin.  This plugin allows splitting the compression method from the write into separate plugin instances.  The scatter plugin goes to multiple (3-4) IMM plugins which do compression.  These are brought together & sorted by the Gather plugin which then feeds an IMM plugin which writes to the file.
 * ADCore 3-4 has a codec plugin which allows for different modes of compression.  These will be tested some in this round.  

NOTES:
 * The HDF plugin in ADCore 3-5 is anticipated to have ability to use the output of the compression from Codec plugin.  IF this can be worked into analysis, this may become the default output method.  This will introduce more commonality than using the IMM plugin which is is specific to APS TRR group.
 * Looking at the 24 bit image number from the driver, we need to make this more robust for this application.  It is anticipated to go to higher rates which may cause rollover of this number in the course of one experiment.  The driver library returns a 32bit image id, but the hardware does 24 bit.  At 2000fps the 24 bit counter will roll over every 2.333 hrs.  This should ideally be fixed between firmware/vendor library to give a 32 bit number.  If not, we should be able to catch the rollover in the AD driver and adjust this.  Going to 32 bits would change rollover frequency to once every 24 days of run time.  Note however that the detector rate is expected to go up by changing image depth.  This could bring the rollover back down to a couple of days.  Need to think about extending the range past 32 bits in the long term.
