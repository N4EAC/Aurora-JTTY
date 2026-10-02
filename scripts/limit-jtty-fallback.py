#!/usr/bin/env python3
"""Constrain the final half-symbol fallback's CRC search budget."""
import pathlib,sys
root=pathlib.Path(sys.argv[1]);p=root/'lib/jtty/jtty_tbcc_decoder.f90'
s=p.read_text();old='    if (decoded_rung%accepted) then\n      call accept_rung(decoded_rung, 4, .true., payload, success, result)'
assert old in s
s=s.replace(old,'''    ! M2 discards phase and is the last-chance observation. Only its best
    ! hypothesis may authorize a decode; scanning weaker CRC-valid list
    ! entries produced an isolated fourth-ranked false acceptance in the
    ! 2026-10-02 field recording. Coherent M1 rungs retain their full lists.
    if (decoded_rung%accepted .and. &
         decoded_rung%result%accepted_hypothesis_rank.eq.1) then
      call accept_rung(decoded_rung, 4, .true., payload, success, result)''')
p.write_text(s)
