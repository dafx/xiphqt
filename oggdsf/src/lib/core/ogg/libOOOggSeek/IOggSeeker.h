class IOggSeeker {

	IOggSeeker();
	virtual ~IOggSeeker();

	typedef pair<__int64, unsigned long> tSeekPair;
	virtual bool buildTable();

	//IOggCallback interface
	virtual bool acceptOggPage(OggPage* inOggPage);

	__int64 fileDuration();

	bool addSeekPoint(__int64 inTime, unsigned long mStartPos);
	tSeekPair getStartPos(__int64 inTime);
	bool enabled();
};