# Fortissimo Protocol
## Background and Introduction
Fortissimo was born out of the SAFRN 1.0 project; conceived as a protocol for composing protocols.
The "Fronctocol Framework" as it was called at the time was tied to a TCP/TLS based protocol for intializing connections as well as synchronizing Fronctocols.
Eventually Fortissimo outgrew SAFRN, and other projects sought to reap the benefits of Fronctocols.

Fortissimo is largely network agnostic, imposing the following two requirements on networking.

 - All messages must be delivered (guaranteed delivery).
 - Any two messages sent from one participant to the same participant must be delivered in the same order which they were sent (pairwise in-order delivery).
 - Each message delivered must have an equivelant message body on the sender as the receiver (correctness).
 - The beginning and end of the message must be preserved on the recipient (preserved boundaries).

Fortissimo interacts with the network via the ``IncomingMessage_T`` and ``OutgoingMessage_T`` interfaces.
The ``IncomingMessage_T`` encapsulates the sender along with the sequence of bytes costituting the message.
Likewise, the ``OutgoingMessage_T`` encapsulates the intended recipient along with a sequence of bytes to send to the recipient.

Initializing connections, verifying identities, and exchanging or distibuting input data is left to an external networking stack which could consist of TCP/TLS, RESTful HTTP Requests, in-process demonstration/testing methods, or carrier pigeons.
**NOTE:** Fortissimo is also agnostic to confidentiality requirements for its connections. If a user of Fortissimo would like confidential connections, it is their own respoinsibility.

## Networking in Fortissimo

The network loop's interface to Fortissimo is through the ``FronctocolsManager`` class, which has two methods of note.

 - ``init(unique_ptr<Fronctocol>, vector<OutogingMessage_T>&)``: Initializes Fortissimo by providing the "main" fronctocol.
   Main is given a fronctocol ID of 0, and is assumed to be peers with its counterpart on all other participants.
   It is also given an empty vector of ``OutgoingMessage_T``.
   This empty vector will be filled with messages to be sent after init is completed.
 - ``handleReceive(IncomingMessage_T&, vector<OutgoingMessage_T>&)``: Processes a single message.
   Again this requires an empty vector of ``OutgoingMessage_T``.
   The ``FronctocolsManager`` will fill this vector, and expect they be delivered to appropriate peers.

### Fronctocol Synchronization and Overhead

Because the fronctocol system, which Fortissimo will manage, will create and complete fronctocols willy nilly, Fortissimo needs to syncrhonize each fronctocol in each participant with its peer fronctocols in other participants.
Fortissimo expects that a new Fronctocol in one participant will be matched by a partner in each of its declared peer participants.
In an n-party environment, a fronctocol may make differing numbers of child fronctocols with different participants, thus a fronctocol on one server may not have the same ID as its partner on another server.

Fortissimo matches each fronctocol with its peers by the fronctocol which invoked them, the peerset ivolved, and finally the order in which they are invoked.
To do this, each participant sends a synchronization message to all other participants.
The fronctocol's ``init`` method is not invoked until all synchronization messages are received.

Likewise, when a fronctocol completes, all participants must agree that it is deleted.
In this case, however, the caller's ``handleComplete`` method can be invoked before all completion messages are received.

### Format of Messages
All messages are prefixed by a single byte control block to declare what type of message, as well as an 8-byte Fronctocol ID, which declares which message it is intended to go to.

### Syncrhonization Message
This message is sent by each participant in a fronctocol to each other participant, and declares that the fronctocol is beginning.

 - 1 byte: control block is 0x00.
 - 8 bytes: Fronctocol ID is that of the calling fronctocol's peer in the receiving participant.
   Currently this is easier to match, since the sender does not necessarily know the receiver's fronctocol ID.
 - n bytes: A serialization of the entire Peer Set involved in the fronctocol.
   The recipient uses this to match the new fronctocol with a child in itself.
 - 8 bytes: Fronctocol ID of the new fronctocol in the sender.
   This declares to the recipient the sender's ID to be used by subsequent messages.

### Payload Message
After syncrhonization, fronctocols can exchange messages, each carrying a payload.

 - 1 byte: control block is 0x01.
 - 8 bytes: the Fronctocol ID of the recipient's fronctocol.
 - n bytes: ``<<blob>>`` of payload.

### Completion Message
Upon completion of a fronctocol, the following completion message is sent.

 - 1 byte: control block is 0x02.
 - 8 bytes: the Fronctocol ID of the recipient's fronctocol.

### Abort Message
In the case of something going awfully wrong, the protocol may be aborted before it completes.
In this event an abort message is sent.

 - 1 byte: control block is 0x04.
 - 8 bytes: The fronctocol ID will always default to invalid (``UINT64_MAX``)
