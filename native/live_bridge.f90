! JTTY Workbench live adapter. GPL-3.0-or-later, 2026-10-01.
module workbench_live
  use iso_c_binding
  use jtty_mod
  use jtty_fec
  use jtty_mdec
  implicit none
  integer, parameter :: LIMIT=180*12000, SHIFT=120*12000, HOP=59*384/4, WINDOW=59*384+HOP
  integer(c_int16_t), save :: storage(LIMIT)=0
  integer, save :: length=0, cursor=1
  real(c_double), save :: time_base=0
contains
  subroutine jw_reset() bind(C)
    integer npatience,nthreads
    common /patience/ npatience,nthreads
    npatience=0
    nthreads=1
    storage=0
    length=0
    cursor=1
    time_base=0
    call reset_decode_search_state()
    call discard_pending_updates()
    next_message_id=1
  end subroutine

  integer(c_int) function jw_encode(text,profile,frequency,samples,capacity,normalized) bind(C)
    character(c_char), intent(in) :: text(* )
    integer(c_int), value :: profile,capacity
    real(c_float), value :: frequency
    integer(c_int16_t), intent(out) :: samples(*)
    character(c_char), intent(out) :: normalized(*)
    character(80) message,rendered
    character(34) blocks(MAX_FRAMES)
    integer frames,i,n,start,payload(PAYLOAD_BITS),symbols(TOTAL_K),tones(59*MAX_FRAMES)
    real, allocatable :: wave(:)
    complex, allocatable :: cwave(:)
    message=''
    jw_encode=-1
    do i=1,80
      if(text(i).eq.c_null_char) exit
      message(i:i)=text(i)
    enddo
    call pack_jtty(message,blocks,frames,profile)
    if(frames.le.0) return
    n=frames*59*384
    if(n.gt.capacity) return
    call unpack_jtty(blocks,frames,rendered)
    do i=1,80
      if(rendered(i:i).eq.'~') rendered(i:i)=' '
      normalized(i)=rendered(i:i)
    enddo
    normalized(81)=c_null_char
    do i=1,frames
      read(blocks(i),'(34i1)') payload
      call tbcc_encode(payload,symbols,JTTY_TBCC_PROFILE_1167_1545_80F)
      start=(i-1)*59+1
      tones(start:start+12)=is13
      tones(start+13:start+58)=symbols
    enddo
    allocate(wave(n),cwave(n))
    call gen_jttywave(tones,frames*59,384,2.0,12000.0,frequency,cwave,wave,0,n)
    samples(1:n)=int(nint(wave*0.7*32767.0),c_int16_t)
    jw_encode=n
  end function

  integer(c_int) function jw_feed(samples,count,frequency,tolerance) bind(C)
    integer(c_int16_t), intent(in) :: samples(*)
    integer(c_int), value :: count
    real(c_float), value :: frequency,tolerance
    integer i
    jw_feed=-1
    if(count.lt.0.or.count.gt.12000) return
    if(length+count.gt.LIMIT) then
      storage(1:length-SHIFT)=storage(SHIFT+1:length)
      length=length-SHIFT
      cursor=cursor-SHIFT
      time_base=time_base+real(SHIFT,c_double)/12000
      do i=1,nactive
        active_messages(i)%tsync=active_messages(i)%tsync-real(SHIFT)/12000
        active_messages(i)%start_tsync=active_messages(i)%start_tsync-real(SHIFT)/12000
      enddo
      do i=1,nrecent
        recent_frames(i)%tsync=recent_frames(i)%tsync-real(SHIFT)/12000
      enddo
    endif
    storage(length+1:length+count)=samples(1:count)
    length=length+count
    do while(cursor+WINDOW-1.le.length)
      call jtty_mdecode_step(storage,length,cursor,1,WINDOW,384,-1, &
        max(200,nint(frequency-tolerance)),min(2800,nint(frequency+tolerance)),frequency,tolerance,4.6)
      cursor=cursor+HOP
    enddo
    jw_feed=0
  end function

  integer(c_int) function jw_pop(identifier,frequency,seconds,complete,text) bind(C)
    integer(c_int64_t), intent(out) :: identifier
    real(c_float), intent(out) :: frequency
    real(c_double), intent(out) :: seconds
    integer(c_int), intent(out) :: complete
    character(c_char), intent(out) :: text(*)
    character(80) message
    integer i
    jw_pop=0
    if(npending.le.0) return
    identifier=pending_updates(pending_first)%message_id
    frequency=pending_updates(pending_first)%f1
    seconds=pending_updates(pending_first)%start_tsync+time_base
    complete=merge(1,0,pending_updates(pending_first)%complete)
    message=display_message_text(pending_updates(pending_first)%decoded)
    do i=1,80
      text(i)=message(i:i)
    enddo
    text(81)=c_null_char
    pending_first=pending_first+1
    npending=npending-1
    if(npending.eq.0) pending_first=1
    jw_pop=1
  end function
end module
