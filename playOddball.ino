void playOddball(int *buf) {
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
    if (metronome_clicks_played <= nSnds) { 
      if (buf[metronome_clicks_played - 1] == 1) {
        // Play metronome click
        sound1.play(AudioSampleSndstandard);      
      }
      else if (buf[metronome_clicks_played - 1] == 2) {
        sound1.play(AudioSampleSndtargetlow);
      }
      else {
        sound1.play(AudioSampleSndtargethigh);      
      }
    }
    
    // And schedule the next upcoming metronome click
    next_metronome_t += metronome_interval;  
    }
  }
}


void setupOddball(int nSnds, int nTarget) {
    // Free existing memory if allocated
    if (buf != NULL) free(buf);
    if (bufIndex != NULL) free(bufIndex);
    Serial.print("In setupOddball(): "); Serial.println(nSnds);
    Serial.print("In setupOddball(): "); Serial.println(nTarget);
    
    // Allocate new memory
    buf = (int*)malloc(nSnds * sizeof(int));
    bufIndex = (int*)malloc(nSnds * sizeof(int));
    
    if (!buf || !bufIndex) {
        Serial.println("Memory allocation failed!");
        if (buf) free(buf);
        if (bufIndex) free(bufIndex);
        buf = NULL;
        bufIndex = NULL;
        return;
    }
  
  Serial.print("nSnds: "); Serial.println(nSnds);
  Serial.print("nTarget: "); Serial.println(nTarget);
  
  // fill beats with ones, and beatIndex with its index.
  for(int i = 0; i < nSnds; ++i) {buf[i] = 1; bufIndex[i] = i;}

//  int nSplit = rand() % (nTarget + 1);
//  int nSplit[1]; // nSplit needs to be defined as an array of one element so that randperm is happy.
//  randperm(nSplit,nTarget,1);
//  int splitOK = 0;
//  Serial.print("nSplit: "), Serial.println(nSplit[0]); 
//  Serial.println();  
//  while (splitOK != 1) {
//      if (nSplit[0] > (nTarget/2) + (nTarget/4)){
////          nSplit = random(0, nTarget); 
//          randperm(nSplit,nTarget,1);
//      }
//      else if (nSplit[0] < (nTarget/2) - (nTarget/4)) {
////          nSplit = random(0, nTarget);
//          randperm(nSplit,nTarget,1);
//      }
//      else {
//          splitOK = 1;
//      }
//  }

  // Fill the 1x20 vector with random 2s and 3s
  int vector20[nTarget];
  for (int i = 0; i < nTarget; i++) vector20[i] = random(2, 4);

  // Shuffle indices
  shuffle(bufIndex,nSnds);

  // Place 20 values while ensuring no isolated 1s
  int placed = 0;
  
  // Place as many as possible randomly
  for (int i = 0; i < nSnds && placed < nTarget; i++) {
      int pos = bufIndex[i];
      if (buf[pos] == 1 && is_valid_placement(buf, pos, nSnds)) {
          buf[pos] = vector20[placed];
          placed++;
      }
  }
  
    // Force remaining placements with safeguards
    int force_attempts = 0;
    const int max_force_attempts = nSnds * 5; // Limit iterations
    while (placed < nTarget && force_attempts < max_force_attempts) {
        int prev_placed = placed;
        for (int pos = 0; pos < nSnds && placed < nTarget; pos++) {
            if (buf[pos] == 1 && is_valid_placement(buf, pos, nSnds)) {
                buf[pos] = vector20[placed];
                placed++;
            }
        }
        force_attempts += nSnds;
        
        // Break if no progress or close to target
        if (placed == prev_placed || (placed >= nTarget - 2 && placed < nTarget)) {
            break;
        }
    }

    // Print results using sprintf
    char buffer[128]; // Buffer for formatting
    
    if (placed < nTarget) {
        sprintf(buffer, "Placed %d of %d values after %d attempts", placed, nTarget, force_attempts);
    } else {
        sprintf(buffer, "Placed all %d values in %d attempts", nTarget, force_attempts);
    }
    Serial.println(buffer);
    
    Serial.println("Original 1x20 vector:");
    for (int i = 0; i < nTarget; i++) {
        sprintf(buffer, "%d ", vector20[i]);
        Serial.print(buffer);
    }
    Serial.println("\n");
    
    Serial.print("Final 1x");
    Serial.print(nSnds);
    Serial.println(" vector:");
    for (int i = 0; i < nSnds; i++) {
        sprintf(buffer, "%d ", buf[i]);
        Serial.print(buffer);
        if ((i + 1) % 10 == 0) Serial.println();
    }
    Serial.println();
    
    int isolated_ones = 0, adjacent_twos_threes = 0;
    for (int i = 0; i < nSnds; i++) {
        if (buf[i] == 1) {
            if ((i == 0 || buf[i - 1] != 1) && 
                (i == nSnds - 1 || buf[i + 1] != 1)) {
                isolated_ones++;
            }
        }
        if (i < nSnds - 1 && buf[i] != 1 && buf[i + 1] != 1) {
            adjacent_twos_threes++;
        }
    }
    sprintf(buffer, "Isolated 1s: %d (should be 0)", isolated_ones);
    Serial.println(buffer);
    sprintf(buffer, "Adjacent 2s/3s: %d (should be 0)", adjacent_twos_threes);
    Serial.println(buffer);

    // Overwrite first 15 positions with 1s
    for (int i = 0; i < 15; i++) {
        buf[i] = 1;
    }
    
    // Print final buf after overwrite
    Serial.print("Final 1x");
    Serial.print(nSnds);
    Serial.println(" vector (after overwrite):");
    for (int i = 0; i < nSnds; i++) {
        sprintf(buffer, "%d ", buf[i]);
        Serial.print(buffer);
        if ((i + 1) % 10 == 0) Serial.println();
    }
    Serial.println();    
}


// Function to shuffle an array (Fisher-Yates shuffle)
void shuffle(int arr[], int size) {
    for (int i = size - 1; i > 0; i--) {
        int j = random(0, i + 1);
        int temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
    }
}

// Check if placing a value at pos creates an isolated 1
int is_valid_placement(int vec[], int pos, int size) {
    if (size < 3) return 1;
    
    if (pos > 0 && vec[pos - 1] == 1) {
        if (pos == 1 || (pos > 1 && vec[pos - 2] != 1)) return 0;
    }
    if (pos < size - 1 && vec[pos + 1] == 1) {
        if (pos == size - 2 || (pos < size - 2 && vec[pos + 2] != 1)) return 0;
    }
    
    if (pos > 0 && vec[pos - 1] != 1) return 0;
    if (pos < size - 1 && vec[pos + 1] != 1) return 0;
    
    return 1;
}
