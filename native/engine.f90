! JTTY Workbench, created 2026-10-01. GPL-3.0-or-later.
! Standalone adapter around unmodified WSJT-X JTTY routines.
program jtty_engine
  use iso_fortran_env, only: int16, error_unit
  use jtty_mod
  use jtty_fec
  use jtty_mdec
  implicit none
  character(len=16) :: mode
  character(len=4096) :: path, argument
  character(len=80) :: message, normalized
  character(len=34) :: blocks(MAX_FRAMES)
  integer :: frames, profile, i, start, count, total, unit, stat, file_bytes
  integer :: tones(59*MAX_FRAMES), payload(PAYLOAD_BITS), symbols(TOTAL_K)
  integer :: npatience, nthreads
  common /patience/ npatience,nthreads
  real :: frequency, tolerance
  real, allocatable :: waveform(:)
  complex, allocatable :: complex_wave(:)
  integer(int16), allocatable :: pcm(:)
  npatience=0
  nthreads=1
  call get_command_argument(1,mode)
  call get_command_argument(2,path)
  call get_command_argument(3,argument)
  read(argument,*,iostat=stat) frequency
  if(stat.ne.0) call fail('Invalid frequency')
  select case(trim(mode))
  case('encode')
    call get_command_argument(4,argument)
    read(argument,*,iostat=stat) profile
    if(stat.ne.0) call fail('Invalid profile')
    call get_command_argument(5,argument)
    if(len_trim(argument).gt.80) call fail('Message exceeds 80 characters')
    message=argument
    call pack_jtty(message,blocks,frames,profile)
    if(frames.le.0) call fail('Empty or unencodable message')
    call unpack_jtty(blocks,frames,normalized)
    do i=1,len(normalized)
      if(normalized(i:i).eq.'~') normalized(i:i)=' '
    enddo
    do i=1,frames
      read(blocks(i),'(34i1)') payload
      call tbcc_encode(payload,symbols,JTTY_TBCC_PROFILE_1167_1545_80F)
      start=(i-1)*59+1
      tones(start:start+12)=is13
      tones(start+13:start+58)=symbols
    enddo
    count=frames*59*384
    ! Half a second of silence on each side makes offline acquisition reliable.
    allocate(waveform(count),complex_wave(count),pcm(count+12000))
    call gen_jttywave(tones,frames*59,384,2.0,12000.0,frequency, &
         complex_wave,waveform,0,count)
    pcm=0
    pcm(6001:6000+count)=int(nint(0.7*32767.0*waveform),int16)
    open(newunit=unit,file=trim(path),status='replace',access='stream',iostat=stat)
    if(stat.ne.0) call fail('Cannot open output')
    write(unit) pcm
    close(unit)
    write(*,'(i0,a,a)') frames,achar(9),trim(normalized)
  case('decode')
    call get_command_argument(4,argument)
    read(argument,*,iostat=stat) tolerance
    if(stat.ne.0) call fail('Invalid tolerance')
    inquire(file=trim(path),size=file_bytes,iostat=stat)
    if(stat.ne.0.or.file_bytes.le.0.or.mod(file_bytes,2).ne.0) call fail('Invalid PCM input')
    total=file_bytes/2
    if(total.gt.180*12000) call fail('Recording exceeds 180 seconds')
    count=59*384+59*384/4
    allocate(pcm(total+count))
    pcm=0
    open(newunit=unit,file=trim(path),status='old',access='stream',iostat=stat)
    if(stat.ne.0) call fail('Cannot open recording')
    read(unit,iostat=stat) pcm(1:total)
    close(unit)
    if(stat.ne.0) call fail('Cannot read recording')
    call reset_decode_search_state()
    call discard_pending_updates()
    do start=1,total,59*384/4
      call jtty_mdecode_step(pcm,size(pcm),start,1,count,384,-1, &
        max(200,nint(frequency-tolerance)),min(2800,nint(frequency+tolerance)),frequency,tolerance,4.6)
      do i=pending_first,pending_first+npending-1
        write(*,'(i0,a,f8.2,a,f9.3,a,i0,a,a)') pending_updates(i)%message_id,achar(9), &
          pending_updates(i)%f1,achar(9),pending_updates(i)%start_tsync,achar(9), &
          merge(1,0,pending_updates(i)%complete),achar(9), &
          trim(display_message_text(pending_updates(i)%decoded))
      enddo
      call discard_pending_updates()
    enddo
    call jtty_release_fft_resources()
  case default
    call fail('Usage: jtty-engine encode|decode PCM frequency profile|tolerance [message]')
  end select
contains
  subroutine fail(reason)
    character(len=*), intent(in) :: reason
    write(error_unit,'(a)') reason
    stop 1
  end subroutine fail
end program jtty_engine
