#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ogg2/ogg.h>
#include "codec.h"

extern int codebook_p;
extern int headerinfo_p;
extern int warn_p;

/* Header packing/unpacking ********************************************/

static int _vorbis_unpack_info(vorbis_info *vi,oggpack_buffer *opb){
  unsigned long version;
  unsigned long channels;
  unsigned long rate;
  long bitrate_upper;
  long bitrate_nominal;
  long bitrate_lower;
  unsigned long ret;
  
  oggpack_read(opb,32,&version);
  oggpack_read(opb,8,&channels);
  oggpack_read(opb,32,&rate);
  oggpack_read(opb,32,&bitrate_upper);
  oggpack_read(opb,32,&bitrate_nominal);
  oggpack_read(opb,32,&bitrate_lower);

  oggpack_read(opb,4,&ret);
  vi->blocksizes[0]=1<<ret;
  oggpack_read(opb,4,&ret);
  vi->blocksizes[1]=1<<ret;

  ret=0;
  if(headerinfo_p){
    printf("info header: Vorbis identification header parsed:\n"
	   "             Stream version     : %lu\n"
	   "             Output channels    : %lu\n"
	   "             Output sample rate : %lu Hz\n",
	    version,channels,rate);
    printf("             Bitrate targets    : ");
    bitrate_lower<=0?printf("unset/"):printf("%ld/",bitrate_lower);
    bitrate_nominal<=0?printf("unset/"):printf("%ld/",bitrate_nominal);
    bitrate_upper<=0?printf("unset\n"):printf("%ld\n",bitrate_upper);
    printf("             Block sizes        : %d/%d samples\n\n",
	   vi->blocksizes[0],vi->blocksizes[1]);
  }
  if((warn_p || headerinfo_p) && rate<64000 && vi->blocksizes[1]>2048){
    printf("WARN header: blocksizes greater than 2048 are limited to\n"
	   "             sample rates over or equal to 64kHz.\n\n");
    ret=1;
  }

  if((warn_p || headerinfo_p) && rate<1){
    printf("WARN header: declared sample rate is invalid\n\n");
    ret=1;
  }

  if((warn_p || headerinfo_p) && channels<1){
    printf("WARN header: declared number of channels is invalid\n\n");
    ret=1;
  }

  if((warn_p || headerinfo_p) && vi->blocksizes[0]<64){
    printf("WARN header: short block sizes less than 64 samples are invalid.\n\n");
    ret=1;
  }

  if((warn_p || headerinfo_p) && vi->blocksizes[1]<vi->blocksizes[0]){
    printf("WARN header: long blocks may not be shorter thans hort blocks.\n\n");
    ret=1;
  }
  
  if((warn_p || headerinfo_p) && vi->blocksizes[1]>8192){
    printf("WARN header: long blocks may not exceed 8192 samples.\n\n");
    ret=1;
  }

  if((warn_p || headerinfo_p) && oggpack_eop(opb)){
    printf("WARN header: premature end of packet.\n\n");
    ret=1;
  }

  if((warn_p || headerinfo_p) && ret==1)
    printf("WARN header: invalid stream declaration; do not decode this stream.\n\n");
  
  return(ret);
}

static int _vorbis_unpack_comment(oggpack_buffer *opb){
  unsigned long i,j;
  unsigned long temp;
  unsigned long len;
  unsigned long comments;

  if(headerinfo_p)
    printf("info header: Vorbis comment header parsed:\n");
  oggpack_read(opb,32,&len);
  if(oggpack_eop(opb))goto err_out;
  if(headerinfo_p)
    printf("             vendor length   : %lu\n",len);
  if(headerinfo_p)
    printf("             vendor string   : \"");
    
  for(i=0;i<len;i++){
    oggpack_read(opb,8,&temp);
    if(headerinfo_p)
      putchar((int)temp);
  }
  if(headerinfo_p){
    putchar('"');
    putchar('\n');
  }

  oggpack_read(opb,32,&comments);
  if(oggpack_eop(opb))goto err_out;
  if(headerinfo_p)
    printf("             total comments  : %lu (comments follow)\n\n",
	   comments);
  for(j=0;j<comments;j++){
    oggpack_read(opb,32,&len);
    if(headerinfo_p)
      printf("             comment %lu length: %lu\n",j,len);

    if(oggpack_eop(opb))goto err_out;
    if(headerinfo_p)
      printf("             comment %lu key   : \"",j);
    for(i=0;i<len;i++){
      oggpack_read(opb,8,&temp);
      if(oggpack_eop(opb))goto err_out;
      if(temp=='='){
	i++;
	break;
      }
      if(headerinfo_p)
	putchar(temp);
    }
    if(headerinfo_p)
      printf("\"\n             comment %lu value : \"",j);
    for(;i<len;i++){
      oggpack_read(opb,8,&temp);
      if(oggpack_eop(opb))goto err_out;
      if(headerinfo_p)
	putchar(temp);
    }
    if(headerinfo_p)
      printf("\"\n\n");
  }  

  return(0);
 err_out:
  if(headerinfo_p || warn_p)
    printf("\nWARN header: header hit EOP prematurely.\n\n");
  return(0);
}

static int _vorbis_unpack_books(vorbis_info *vi,oggpack_buffer *opb){
  int i;
  unsigned long ret;

  if(headerinfo_p)
    printf("info header: Vorbis I setup header parsed\n\n");

  /* codebooks */
  oggpack_read(opb,8,&ret);
  vi->books=ret+1;
  if(headerinfo_p)
    printf("info header: Codebooks: %d\n\n",vi->books);
  for(i=0;i<vi->books;i++){
    if(codebook_p)
      printf("info codebk: Parsing codebook %d\n",i);
    if(vorbis_book_unpack(opb,vi->book_param+i))goto err_out;
  }
  if(codebook_p)
    printf("\n");

  /* time backend settings, not actually used */
  oggpack_read(opb,6,&ret);
  i=ret;
  for(;i>=0;i--){
    oggpack_read(opb,16,&ret);
    if(ret!=0){
      if(headerinfo_p || warn_p)
	printf("WARN header: Time %d is an illegal type (%lu).\n\n",
	       i,ret);
    }
  }
  if(oggpack_eop(opb))goto eop;

  /* floor backend settings */
  oggpack_read(opb,6,&ret);
  vi->floors=ret+1;
  if(headerinfo_p)
    printf("info header: Floors: %d\n\n",vi->floors);
  for(i=0;i<vi->floors;i++){
    if(headerinfo_p)
      printf("info header: Parsing floor %d\n",i);
    if(floor_info_unpack(vi,opb,vi->floor_param+i)){
      if(warn_p || headerinfo_p)
	printf("WARN header: Invalid floor; Vorbis stream not decodable.\n");
      goto err_out;
    }
  }
  if(headerinfo_p)
    printf("\n");
  
#if 0
  /* residue backend settings */
  oggpack_read(opb,6,&ret);
  vi->residues=ret+1;
  for(i=0;i<vi->residues;i++)
    if(res_unpack(vi->residue_param+i,vi,opb,print))goto err_out;

  /* map backend settings */
  oggpack_read(opb,6,&ret);
  vi->maps=ret+1;
  for(i=0;i<vi->maps;i++){
    oggpack_read(opb,16,&ret);
    if(ret!=0){
      if(headerinfo_p || warn_p)
	printf("WARN header: Map %d is an illegal type (%lu).\n\n",
	       i,ret);
      goto err_out;
    }
    if(mapping_info_unpack(vi->map_param+i,vi,opb,print))goto err_out;
  }
  
  /* mode settings */
  oggpack_read(opb,6,&ret);
  vi->modes=ret+1;
  for(i=0;i<vi->modes;i++){
    oggpack_read(opb,1,&ret);
    if(oggpack_eop(opb))goto eop;
    vi->mode_param[i].blockflag=ret;

    oggpack_read(opb,16,&ret);
    if(ret){
      if(headerinfo_p || warn_p)
	printf("WARN header: Window in map %d is an illegal type (%lu).\n\n",
	       i,ret);
      goto err_out;
    }
    oggpack_read(opb,16,&ret);
    if(ret){
      if(headerinfo_p || warn_p)
	printf("WARN header: Transform in map %d is an illegal type (%lu).\n\n",
	       i,ret);
      goto err_out;
    }
    oggpack_read(opb,8,&ret);
    vi->mode_param[i].mapping=ret;
    if(vi->mode_param[i].mapping>=vi->maps){
      if(headerinfo_p || warn_p)
	printf("WARN header: Mode %d requests mapping %lu when highest\n"
	       "             numbered mapping is %d.\n\n",
	       i,ret,vi->maps-1);
      goto err_out;
    }
  }
#endif
  
  if(oggpack_eop(opb))goto eop;
  
  return(0);
 eop:
  if(headerinfo_p || warn_p)
    printf("WARN header: Premature EOP reading setup header.\n\n");
 err_out:
  return 1;
}

int vorbis_info_headerin(vorbis_info *vi,ogg_packet *op){
  oggpack_buffer *opb=alloca(oggpack_buffersize());
  
  if(op){
    oggpack_readinit(opb,op->packet);

    /* Which of the three types of header is this? */
    /* Also verify header-ness, vorbis */
    {
      unsigned long temp[6];
      unsigned long packtype;
      int i;

      oggpack_read(opb,8,&packtype);
      for(i=0;i<6;i++)
	oggpack_read(opb,8,temp+i);
      
      if(temp[0]!='v' ||
	 temp[1]!='o' ||
	 temp[2]!='r' ||
	 temp[3]!='b' ||
	 temp[4]!='i' ||
	 temp[5]!='s'){
	/* not a vorbis header */
	if(headerinfo_p || warn_p)
	  printf("WARN header: Expecting a Vorbis stream header, got\n"
		 "             some other packet type instead. Stream is not\n"
		 "             decodable as Vorbis I.\n\n");
	return(-1);
      }
      switch(packtype){
      case 0x01: /* least significant *bit* is read first */
	if(!op->b_o_s){
	  /* Not the initial packet */
	  if(headerinfo_p || warn_p)
	    printf("WARN header: Packet identifies itself as an identification header,\n"
		   "             but does not occur on an initial stream page.\n"
		   "             Stream is not decodable as Vorbis I.\n\n");
	  return(-1);
	}
	if(vi->blocksizes[1]!=0){
	  /* previously initialized info header */
	  if(headerinfo_p || warn_p)
	    printf("WARN header: Packet identifies itself as an identification header,\n"
		   "             but an identification header has already been parsed.\n"
		   "             Stream is not decodable as Vorbis I.\n\n");
	  return(-1);
	}
	return(_vorbis_unpack_info(vi,opb));

      case 0x03: /* least significant *bit* is read first */
	if(vi->blocksizes[1]==0){
	  /* um... we didn't get the initial header */
	  if(headerinfo_p || warn_p)
	    printf("WARN header: Packet identifies itself as comment header\n"
		   "             but no identification header has been parsed yet.\n"
		   "             Stream is not decodable as Vorbis I.\n\n");
	  return(-1);
	}
	return(_vorbis_unpack_comment(opb));

      case 0x05: /* least significant *bit* is read first */
	if(vi->blocksizes[1]==0){
	  /* um... we didn;t get the initial header yet */
	  if(headerinfo_p || warn_p)
	    printf("WARN header: Packet identifies itself as the setup header\n"
		   "             but no identification header has been parsed yet.\n"
		   "             Stream is not decodable as Vorbis I.\n\n");
	  return(-1);
	}
	
	return(_vorbis_unpack_books(vi,opb));

      default:
	/* Not a valid vorbis header type */
	return(-1);
      }
    }
  }
  return(-1);
}

int vorbis_info_clear(vorbis_info *vi){
  int i;
  for(i=0;i<vi->books;i++)
    vorbis_book_clear(vi->book_param+i);
  memset(vi,0,sizeof(*vi));
  return(0);
}
