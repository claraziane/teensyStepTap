//void readFSR(int typeOfAction, char *nameOfFile) {
void readFSR(int typeOfAction) {
  /* This is the usual activity loop */
  
  /* If this is our first loop ever, initialise the time points at which we should start taking action */
  if (prev_t == 0)           { prev_t = current_t; } // To prevent seeming "lost frames"
//  if (next_metronome_t == 0) { next_metronome_t = current_t+metronome_interval; }
  
//  if (current_t > prev_t) {
    // Main loop tick (one ms has passed)
    
    
    if ((prev_active) && (current_t-prev_t > 1)) {
      // We missed a frame (or more)
      missed_frames += (current_t-prev_t);
    }
    
    
    /*
     * Collect data
     */
    fsrReading = analogRead(fsrAnalogPin);
    
    
    

    /*
     * Process data: has a new tap onset or tap offset occurred?
     */

    if (tap_phase==0) {
      // Currently we are in the tap-off phase (nothing was thouching the FSR)
      
      /* First, check whether actually anything is allowed to happen.
   For example, if a tap just happened then we don't allow another event,
   for example we don't want taps to occur impossibly close (e.g. within a few milliseconds
   we can't realistically have a tap onset and offset).
   Second, check whether this a new tap onset
      */
      if ( (current_t > next_event_embargo_t) && (fsrReading>tap_onset_threshold)) {
        // New Tap Onset
        tap_phase = 1; // currently we are in the tap "ON" phase
        tap_onset_t = current_t;
        // don't allow an offset immediately; freeze the phase for a little while
        next_event_embargo_t = current_t + min_tap_on_duration;
      
        // Schedule the next tap feedback time (if we deliver feedback)
        if (metronome && metronome_clicks_played < metronome_nclicks_predelay) {
          next_feedback_t = current_t; // if we are in the pre-delay period, let's play the feedback sound immediately.
        } else {
          next_feedback_t = current_t + auditory_feedback_delay;
        }
      }
      
    } else if (tap_phase==1) {
      // Currently we are in the tap-on phase (the subject was touching the FSR)
      
      // Check whether the force we are currently reading is greater than the maximum force; if so, update the maximum
      if (fsrReading>tap_max_force) {
        tap_max_force_t = current_t;
        tap_max_force   = fsrReading;
      }
      
      // Check whether this may be a tap offset
      if ( (current_t > next_event_embargo_t) && (fsrReading<tap_offset_threshold)) {

      // New Tap Offset
      
      tap_phase = 0; // currently we are in the tap "OFF" phase
      tap_offset_t = current_t;
      
      // don't allow an offset immediately; freeze the phase for a little while
      next_event_embargo_t = current_t + min_tap_off_duration;
    
      // Send data to the computer!
      send_tap_to_serial(typeOfAction);
//      send_event_to_sdCard(typeOfAction, nameOfFile);
      send_response_to_responseArray(typeOfAction);
    
      // Clear information about the tap so that we are ready for the next tap to occur
      tap_onset_t     = 0;
      tap_offset_t    = 0;
      tap_max_force   = 0;
      tap_max_force_t = 0;
  
      }
      
    }


    /* 
     * Deal with the metronome
    */
//    // Is this a time to play a metronome click?
//    if (metronome && (metronome_clicks_played < metronome_nclicks_predelay + metronome_nclicks)) {
//      if (current_t >= next_metronome_t) {
//
//      // Mark that we have another click played
//      metronome_clicks_played += 1;
//    
//      // Play metronome click
//      sound1.play(AudioSampleMetronome);
//      
//      // And schedule the next upcoming metronome click
//      next_metronome_t += metronome_interval;
//      
//      // Proudly tell the world that we have played the metronome click
////      send_metronome_to_serial();
//      }
//    }

//    if (auditory_feedback) {
//      
//      if ((next_feedback_t != 0) && (current_t >= next_feedback_t)) {
//
//        // Play the auditory feedback (relating to the subject's tap)
//        sound0.play(AudioSampleTap);
//      
//        // Clear the queue, nothing more to play
//        next_feedback_t = 0;
//        
//        // Proudly tell the world that we have played the tap sound
//        send_feedback_to_serial();
//      }
//
//    }
    
    // Update the "previous" state of variables
//    prev_t = current_t;
//  }

}
