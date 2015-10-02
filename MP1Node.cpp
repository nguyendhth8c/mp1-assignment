/**********************************
 * FILE NAME: MP1Node.cpp
 *
 * DESCRIPTION: Membership protocol run by this Node.
 * 				Definition of MP1Node class functions.
 **********************************/

#include "MP1Node.h"

/*
 * Note: You can change/add any functions in MP1Node.{h,cpp}
 */

/**
 * Overloaded Constructor of the MP1Node class
 * You can add new members to the class if you think it
 * is necessary for your logic to work
 */
MP1Node::MP1Node(Member *member, Params *params, EmulNet *emul, Log *log, Address *address) {
	for( int i = 0; i < 6; i++ ) {
		NULLADDR[i] = 0;
	}
	this->memberNode = member;
	this->emulNet = emul;
	this->log = log;
	this->par = params;
	this->memberNode->addr = *address;
}

/**
 * Destructor of the MP1Node class
 */
MP1Node::~MP1Node() {}

/**
 * FUNCTION NAME: recvLoop
 *
 * DESCRIPTION: This function receives message from the network and pushes into the queue
 * 				This function is called by a node to receive messages currently waiting for it
 */
int MP1Node::recvLoop() {
    if ( memberNode->bFailed ) {
    	return false;
    }
    else {
    	return emulNet->ENrecv(&(memberNode->addr), enqueueWrapper, NULL, 1, &(memberNode->mp1q));
    }
}

/**
 * FUNCTION NAME: enqueueWrapper
 *
 * DESCRIPTION: Enqueue the message from Emulnet into the queue
 */
int MP1Node::enqueueWrapper(void *env, char *buff, int size) {
	Queue q;
	return q.enqueue((queue<q_elt> *)env, (void *)buff, size);
}

/**
 * FUNCTION NAME: nodeStart
 *
 * DESCRIPTION: This function bootstraps the node
 * 				All initializations routines for a member.
 * 				Called by the application layer.
 */
void MP1Node::nodeStart(char *servaddrstr, short servport) {
    Address joinaddr;
    joinaddr = getJoinAddress();

    // Self booting routines
    if( initThisNode(&joinaddr) == -1 ) {
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "init_thisnode failed. Exit.");
#endif
        exit(1);
    }

    if( !introduceSelfToGroup(&joinaddr) ) {
        finishUpThisNode();
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Unable to join self to group. Exiting.");
#endif
        exit(1);
    }

    return;
}

/**
 * FUNCTION NAME: initThisNode
 *
 * DESCRIPTION: Find out who I am and start up
 */
int MP1Node::initThisNode(Address *joinaddr) {
	/*
	 * This function is partially implemented and may require changes
	 */
	//int id = *(int*)(&memberNode->addr.addr);
	//int port = *(short*)(&memberNode->addr.addr[4]);

	memberNode->bFailed = false;
	memberNode->inited = true;
	memberNode->inGroup = false;
    // node is up!
	memberNode->nnb = 0;
	memberNode->heartbeat = 0;
	memberNode->pingCounter = TFAIL;
	memberNode->timeOutCounter = -1;
    initMemberListTable(memberNode);

    return 0;
}

/**
 * FUNCTION NAME: introduceSelfToGroup
 *
 * DESCRIPTION: Join the distributed system
 */
int MP1Node::introduceSelfToGroup(Address *joinaddr) {
	MessageHdr *msg;
#ifdef DEBUGLOG
    static char s[1024];
#endif

    if ( 0 == memcmp((char *)&(memberNode->addr.addr), (char *)&(joinaddr->addr), sizeof(memberNode->addr.addr))) {
        // I am the group booter (first process to join the group). Boot up the group
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Starting up group...");
#endif
        memberNode->inGroup = true;
    }
    else {
        size_t msgsize = sizeof(MessageHdr) + sizeof(joinaddr->addr) + sizeof(long) + 1;
        msg = (MessageHdr *) malloc(msgsize * sizeof(char));

        // create JOINREQ message: format of data is {struct Address myaddr}
        msg->msgType = JOINREQ;
        memcpy((char *)(msg+1), &memberNode->addr.addr, sizeof(memberNode->addr.addr));
        memcpy((char *)(msg+1) + 1 + sizeof(memberNode->addr.addr), &memberNode->heartbeat, sizeof(long));

#ifdef DEBUGLOG
        sprintf(s, "Trying to join...");/////////////
        log->LOG(&memberNode->addr, s);
#endif

        // send JOINREQ message to introducer member
        emulNet->ENsend(&memberNode->addr, joinaddr, (char *)msg, msgsize);

        free(msg);
    }


    return 1;

}

/**
 * FUNCTION NAME: finishUpThisNode
 *
 * DESCRIPTION: Wind up this node and clean up state
 */
int MP1Node::finishUpThisNode(){
   /*
    * Your code goes here
    */
}

/**
 * FUNCTION NAME: nodeLoop
 *
 * DESCRIPTION: Executed periodically at each member
 * 				Check your messages in queue and perform membership protocol duties
 */
void MP1Node::nodeLoop() {
    if (memberNode->bFailed) {
    	return;
    }

    // Check my messages
    checkMessages();

    // Wait until you're in the group...
    if( !memberNode->inGroup ) {
    	return;
    }

    // ...then jump in and share your responsibilites!
    nodeLoopOps();

    return;
}

/**
 * FUNCTION NAME: checkMessages
 *
 * DESCRIPTION: Check messages in the queue and call the respective message handler
 */
void MP1Node::checkMessages() {
    void *ptr;
    int size;

    // Pop waiting messages from memberNode's mp1q
    while ( !memberNode->mp1q.empty() ) {
    	ptr = memberNode->mp1q.front().elt;
    	size = memberNode->mp1q.front().size;
    	memberNode->mp1q.pop();
    	recvCallBack((void *)memberNode, (char *)ptr, size);
    }
    return;
}

void MP1Node::updateMember(int id, short port,long heartbeat){
    vector<MemberListEntry>::iterator it = memberNode->memberList.begin();
    for (; it != memberNode->memberList.end(); ++it){
        if (it->id == id && it->port ==port){
            if (heartbeat > it->heartbeat){
                it->setheartbeat(heartbeat);
                it->settimestamp(par->getcurrtime);
            }
            return;
        }
    }
    MemberListEntry memberEntry(id,port,heartbeat,par->getcurrtime);
    membetNode->memberList.push_back(memberEntry);

#ifdef DEBUGLOG
    Address joinaddr;
    memcpy(&joinaddr.addr[0],id,sizeof(int));
    memcpy(&joinaddr.addr[4], &port, sizeof(short));
    log->logNodeAdd(&memberNode->addr,&joinaddr);
#endif
}

void MP1Node::updateMember(MemberListEntry& member){
    updateMember.(member.getid(), member.getport(), member.gethearbeat());
}

int MP1Node::memcpyMemberListEntry(char * data, MemberListEntry& member){

    char * p = data;
    memcpy(p, &member.id, sizeof(int));
    p += sizeof(int);
    memcpy(p , &member.port, sizeof(short));
    p += sizeof(short);
    memcpy(p , &member.heartbeat, sizeof(long));
    p += sizeof(long);
    return p - data;
}
void MP1Node::sendMemberList(const char * label, enum MsgTypes msgType, Address * to)
{
    long members = memberNode->memberList.size();
    size_t msgsize = sizeof(MessageHdr) + sizeof(memberNode->addr.addr) + sizeof(long) + members * (sizeof(int) + sizeof(short) + sizeof(log));

MessageHdr * msg = (MessageHdr *) malloc(msgsize * sizeof(char));
char * data = (char*)(msg + 1);
msg->msgType = msgType;
memcpy(data, &memberNode->addr.addr, sizeof(memberNode->addr.addr));
data += sizeof(memberNode->addr.addr);
char * pos_members = data;
data += sizeof(long);

for (vector<MemberListEntry>::iterator it = memberNode->memberList.begin() ; it != memberNode->memberList.end();){

    if (it != memberNode->memberList.begin()){

        if (par->getcurrtime() - it->timestamp > TREMOVE){
#ifdef DEBUGLOG
            Address joinaddr;
            memcpy(&joinaddr.addr[0], &it->id, sizeof(int));
            memcpy(&joinaddr.addr[4], &it->port, sizeof(short));
            log->logNodeRemove(&memberNode->addr, &joinaddr);

#endif
            members--;
            it = memberNode->memberList.erase(it);
            continue;
        }
        if (par->getcurrtime() - it->timestamp > TFAIL)
        {
            members--;
            ++it;
            continue;
        }
    }
        data += memcpyMemberListEntry(data, *it);
        ++it;
}

        memcpy(pos_members, &members, sizeof(long));
        msgsize = sizeof(MessageHdr) + sizeof(memberNode->addr.addr) + sizeof(long) + members * (sizeof(int) + sizeof(short) + sizeof(log));
        emulNet->ENsend(&memberNode->addr, to, (char *)msg, msgsize);

        free(msg);
}

bool MP1Node::recvJoinReq(void *env, char *data, int size){
    //so sanh kich thuoc goi tin co hop le hay ko.
     if (size < (int)(sizeof(memberNode->addr.addr) + sizeof(long))){
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Message JOINREQ received with size wrong. Ignored.");
#endif
        return false;
     }
     Address joinaddr;//6
     long heartbeat;//8

     memcpy(joinaddr.addr, data, sizeof(memberNode->addr.addr));
     memcpy(&heartbeat, data + sizeof(memberNode->addr.addr), sizeof(long));

     int id = *(int*)(&joinaddr.addr);
     int port = *(short*)(&joinaddr.addr[4]);

     updateMember(id, port, heartbeat);

     sendMemberList("JOINREP", JOINREP, &joinaddr);
     return true;
}

bool MP1Node::recvMemberList(const char * label, void *env, char *data, int size)
{
    if (size < (int) (sizeof(long)))
    {
        #ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Message %s received with size wrong. Ignored. size[%s]", label, size);
        #endif
            return false;
    }
    long members;
    memcpy(&members, data, sizeof(long));
    data +=sizeof(long);
    size -=sizeof(long);

    if (size < (int)(members * (sizeof(int) + sizeof(short) + sizeof(log))))
    {
        #ifdef DEBUGLOG
            log->LOG(&memberNode->addr, "Message %s received with size wrong. Ignored.", label);

        #endif 
                return false;
    }

    MemberListEntry member;
        for (long i=0; i<members; i++)
        {
            memcpy(&member.id, data, sizeof(int));
            data += sizeof(int);
            memcpy(&member.port, data, sizeof(short));
            data += sizeof(short);
            memcpy(&member.heartbeat, data, sizeof(long));
            data += sizeof(long);
            member.timestamp = par->getcurrtime();
            updateMember(member);

        }

        return true;
}

bool MP1Node::recvJoinRep(void *env, char *data, int size)
{
    if (size < (int)(sizeof(memberNode->addr.addr)))
    {   
    #ifdef
        log->LOG(&memberNode->addr, "Message JOINREP received with size wrong. Ignored. size[%i]", size);
    #endif
        return false;
    }

    Address senderAddr;
    memcpy(senderAddr.addr, data, sizeof(memberNode->addr.addr));
    data += sizeof(memberNode->addr.addr);
    size -= sizeof(memberNode->addr.addr);

    if (!recvMemberList("JOINREP", env, data, size))
    {
        return false;
    }

    memberNode->inGroup = true;
    return true;
}


bool MP1Node::recvHeartbeatReq(void *env, char *data, int size){
    
}


bool MP1Node::recvHeartbeatRep(void *env, char *data, int size)
{
    if (size < (int)(sizeof(memberNode->addr.addr)))
    {
        #ifdef DEBUGLOG
            log->LOG(&memberNode->addr, "Message HEARTBEATREP received with size wrong. Ignored.");
        #endif
            return false;
    }
    Address senderAddr;
    memcpy(senderAddr.addr, data, sizeof(memberNode->addr.addr));

    int id = *(int*)(&senderAddr.addr);
    int port = *(short*)(&senderAddr.addr[4]);
    vector<MemberListEntry>::iterator it = memberNode->memberList.begin();
    for (++it; it != memberNode->memberList.end(); ++it)
    {
        if (it->id == id && it->port == port)
        {
            it->heatbeat++;
            it->timestamp = par->getcurrtime();

            return true;
        }
    }

    #ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Message HEARTBEATREP not found in member list.");
    #endif
        return false;
}



/**
 * FUNCTION NAME: recvCallBack
 *
 * DESCRIPTION: Message handler for different message types
 */
bool MP1Node::recvCallBack(void *env, char *data, int size ) {
	/*
	 * Your code goes here
	 */
     if (size < (int)sizeof(MessageHdr)) {
    #if DEBUGLOG
    log->LOG(&memberNode->addr, "Message received with size less than MessageHdr. Ignored.");  
    #endif
        return false;  
     }
    MessageHdr * msg = (MessageHdr *)data;
    switch (msg->msgType) {
        case JOINREQ:
            return recvJoinReq(env, data + sizeof(MessageHdr), size - sizeof(MessageHdr));
        case JOINREP:
            return recvJoinRep(env, data + sizeof(MessageHdr), size - sizeof(MessageHdr));
        case HEARTBEATREQ:
            return recvHeartbeatReq(env, data + sizeof(MessageHdr), size - sizeof(MessageHdr));
        case  
            return recvHeartbeatRep(env, data + sizeof(MessageHdr), size - sizeof(MessageHdr));
        case DUMMYLASTMSGTYPE:
            return false;

	/* assert(size >= sizeof(MessageHdr));
	MessageHdr* msg = (MessageHdr*) data;
	Address  *src_addr = (Address * )(msg + 1);
	
	size -=sizeof(MessageHdr) + sizeof(Address) + 1;
	data +=sizeof(MessageHdr) + sizeof(Address) + 1;
	
	switch (msg->msgType){
	case JOINREQ:
		onJoin(src_addr, data ,size);
		onHeartbeat(src_addr, data, size);
		break;
	case PING:
		onHeartbeat(src_addr, data,size);
		break;
	case JOINREP:
	{
		memberNode->inGroup = 1;
		stringstream msg;
		msg <<"JOIN tu" << src_addr->getAddress();
		msg << "data" << *(long*)(data);
		log->LOG(&memberNode->addr,msg.str().c_str());
		onHeartbeat(src_addr, data, size);
		break;
		}
	default:
		log->LOG(&memberNode->addr, "Received other msg");
		break;
	} */

}
/*	Address AddressFromMLE(MemberListEntry* mle) {
		Address a;
		memcpy(a.addr, &mle->id, sizeof(int));
		memcpy(&a.addr[4], &mle->port, sizeof(short));
		return a;
	} */
/*void MP1Node::onJoin(Address* addr, void* data, size_t size){
	MessageHdr* msg;
	size_t msgsize = sizeof(MessageHdr) + sizeof (memberNode->addr) + sizeof(long) + 1;
    msg = (MessageHdr *) malloc(msgsize *sizeof(char));
    msg->msgType = JOINREP;
    memcpy((char *)(msg + 1), &memberNode->addr, sizeof(memberNode->addr));
    memcpy((char *)(msg+1) + sizeof(memberNode->addr) + 1, &memberNode->heartbeat, sizeof(long));
    stringstream ss;
    ss<<"Sending JOINREP to" <<addr->getAddress()<<"heartbeat" <<memberNode->heartbeat;
    log->LOG(&memberNode->addr, ss.str().c_str());
    emulNet->ENsend(&memberNode->addr,addr,(char *) msg,msgsize);
    free(msg);
} */
/*void MP1Node::onHeartbeat(Address* addr, void* data, size_t size){
    std::stringstream msgs;
    assert(size >=sizeof(long));
    long *heartbeat = (long*) data;
    bool newData = UpdateMemberList(addr, *heartbeat);
    if (newData)
    {
        LogMemberList();
        SendHBSomewhere(addr, *heartbeat);
  
    }
} */

/*bool MP1Node::UpdateMemberList(Address *addr, long heartbeat){
    Vector<MemberListEntry>::iterator it;
        for(it = memberNode->memberList.begin(); it != memberNode->memberList.end(); it++){
            if ((AddressFromMLE(&(*it)) == *addr) == 0)
            {
               if (heartbeat > it->gethearbeat())
               {
                   it->setheartbeat(heartbeat);
                   it->settimestamp(par->getcurrtime());
                   return true;
               }
               else{
                return false;
               }
            }
        }

MemberListEntry mle(*((int*)addr->addr),
                            *((short*)&(addr->addr[4])),
                            heartbeat,
                            par->getcurrtime());
memberNode->memberList.push_back(mle);
        log->logNodeAdd(&memberNode->addr, addr);
        return true;

}*/

/*void MP1Node::LogMemberList(){
    stringstream msg;
    msg << "[";
        for (vector<MemberListEntry>::iterator it = memberNode->memberList.begin(); it != memberNode->memberList.end();it++)
        {
            msg <<it>>getid() << ":" <<it->gethearbeat()<< "("<<it->gettimestamp<< "), ";
        }
        msg<<"]";
}*/

/*void MP1Node::SendHBSomewhere(Address *src_addr, long heartbeat){
    int k=30;
        double prob = k / (double)memberNode->memberList.size();
        MessageHdr *msg;
    size_t msgsize = sizeof(MessageHdr) + sizeof(src_addr->addr) + sizeof(long) + 1;
    msg = (MessageHdr * ) malloc(msgsize * sizeof(char));
    //tao JOINREQ de dinh dang du lieu
    msg->msgType = PING;
    memcpy((char *)(msg+1), src_addr->addr, sizeof(src_addr->addr));
    memcpy((char *)(msg+1) + sizeof(src_addr->addr) + 1, &heartbeat, sizeof(long));
    for (vector<MemberListEntry>::iterator it = memberNode->memberList.begin(); it != memberNode->memberList.end(); it++)
    {
        Address dst_addr = AddressFromMLE(&(*it));
            if ((dst_addr == memberNode->addr)==0 || ((dst_addr == *src_addr) == 0))
            {
                continue;
            }
            if ((((double)(rand() % 100))/100) < prob)
            {
                emulNet->ENsend(&memberNode->addr, &dst_addr, (char *)msg, msgsize);
            }
    }
    free(msg);
} */
}
/**
 * FUNCTION NAME: nodeLoopOps
 *
 * DESCRIPTION: Check if any node hasn't responded within a timeout period and then delete
 * 				the nodes
 * 				Propagate your membership list
 */
void MP1Node::nodeLoopOps() {

	/*
	 * Your code goes here
	 */

    return;
}

/**
 * FUNCTION NAME: isNullAddress
 *
 * DESCRIPTION: Function checks if the address is NULL
 */
int MP1Node::isNullAddress(Address *addr) 
{
	return (memcmp(addr->addr, NULLADDR, 6) == 0 ? 1 : 0);
}

/**
 * FUNCTION NAME: getJoinAddress
 *
 * DESCRIPTION: Returns the Address of the coordinator
 */
Address MP1Node::getJoinAddress() 
{
//tra ve 1 dia chi.
    Address joinaddr;

    memset(&joinaddr, 0, sizeof(Address));
    *(int *)(&joinaddr.addr) = 1;//gan ip 1.0.0.0
    *(short *)(&joinaddr.addr[4]) = 0;//gan port 0;

    return joinaddr;
}

/**
 * FUNCTION NAME: initMemberListTable
 *
 * DESCRIPTION: Initialize the membership list
 */
void MP1Node::initMemberListTable(Member *memberNode) {
//khoi tao nen table node.
    memberNode->memberList.clear();
    MemberListEntry memberEntry(id, port, 0, par->getcurrtime());//id, port,heatbeat,time.
    memberNode->memberList.push_back(memberEntry);
    memberNode->myPos = memberNode->memberList.begin();

  //  MemberListEntry mle = MemberListEntry(id,port);
  //  mle.settimestamp(par->getcurrtime());
  //  mle.setheartbeat(memberNode->heartbeat);
//    memberNode->memberList.push_back(mle);
} 

/**
 * FUNCTION NAME: printAddress
 *
 * DESCRIPTION: Print the Address
 */
void MP1Node::printAddress(Address *addr)
{
    printf("%d.%d.%d.%d:%d \n",  addr->addr[0],addr->addr[1],addr->addr[2],
                                                       addr->addr[3], *(short*)&addr->addr[4]) ;    
}
