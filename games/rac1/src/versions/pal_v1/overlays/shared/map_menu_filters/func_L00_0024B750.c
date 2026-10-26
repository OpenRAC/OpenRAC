void AdvanceCardMenu(void) {
 int old=mode_freeze_flags;
 int intermediate=old&~4;
 int flags=intermediate&~2;
 mode_freeze_flags=flags;
 if(memory_card_state[0xF4/4]==0) mode_freeze_state=3;
 else if(old&0x80) { mode_freeze_state=21; mode_freeze_flags=(flags^0x80)|0x40; }
 else if(old&0x100) { mode_freeze_state=20; mode_freeze_flags=(flags^0x100)|0x40; }
 else if(memory_card_state[7]!=0) { memory_card_state[0xF4/4]=0; mode_freeze_flags=flags|1; mode_freeze_state=2; }
 else if(old&0x200) { mode_freeze_flags=flags^0x200; mode_freeze_state=22; }
}
