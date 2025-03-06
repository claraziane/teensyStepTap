void playPureTone() {
  /*This comes from teensytap directly. 
   We probably don't need metronome_nclicks_predelay (ask Clara). 
   This servers to schedule the auditory feedback which we will not have (I think, ask CZ)
   */

   if (next_metronome_t == 0) { next_metronome_t = current_t + metronome_interval; }

  /* 
   * Deal with the metronome
  */
  // Is this a time to play a metronome click?
//  if (metronome && (metronome_clicks_played < metronome_nclicks_predelay + metronome_nclicks)) {
  if (metronome) { //AN: changed this to allow for infinite clicks until active = false.
    if (current_t >= next_metronome_t) {

    // Mark that we have another click played
    metronome_clicks_played += 1;
  
    // Play metronome click
    sound1.play(AudioSampleSndstandard);
    
    // And schedule the next upcoming metronome click
    next_metronome_t += metronome_interval;
    
    // Proudly tell the world that we have played the metronome click
//      send_metronome_to_serial();
    }
  }
}
