#ifndef OFFER_ADDRESS_MESSAGE
#define OFFER_ADDRESS_MESSAGE

#include <jaus.h>

class OfferAddressMessage : JudpTransportMessage
{
public:
	OfferAddressMessage(JausByte offeredId, JausUnsignedShort leaseTimeMin);	
	~OfferAddressMessage(void);
	
	unsigned char* toBuffer(void);
	bool fromBuffer(unsigned char *);
	
	JausByte getOfferedId(void);
	JausUnsignedShort getLeaseTime(void);
	
private:
	JausByte offeredId;
	JausUnsignedShort leaseTimeMin;	
};

#endif
