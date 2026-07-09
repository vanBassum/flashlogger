This is like a logbook, when i think of something i just note it down. When you read something, it could be superseded later. 



First lets note down some constraints and or goals:

No dynamic memory at all
use std int for unambiguty
Keep the api simple
needs to be threadsafe
We need:
- Append
- Read
- Overwrite (only clear bits)

A logrecord consists of multiple key value pairs.
The size of the key and value is configurable when formatting the flash. After that its static.

Design choise to be made:
A logrecord could be static, for example always 5 key value pairs.
- Simple, and indexes work.
- More unused space.

Logrecord could have a start record.
- Not as simple, indexes dont work. We need iterators.
- More efficient, no gaps in the flash.
- Cant determine up front how many items will fit. 

We need good names for:
- A record or item. (multiple key value pairs)
- A key value pair, this is annoying name.

Name needs to be generic enough to fit in all cases. But should not be so generic that it collides with existing things.

I think we arrive at the point where we need to define names. Give some options


Record and Field!



So, lets say the user defines key to be 1 byte and a value to be 4 bytes.
Each field will be 5 bytes (not taking into account perhaps some overhead for a header or smt)

Lets say we want to write, A thermostat that reports its temperature.
- Timestamp: 2026-05-12 10.30.00
- DeviceType: Thermostat
- LogCode: TemperatureChanged	(Each device has its own logcodes)
- DeviceId: 5 				
- Measurement: 18.9 C

We dont have 8 bytes to store the timestamp, so we can repeat the same key in order to write bigger values.

In flash it will look like:
0x01 0x12 0x34 0x56 0x78		// 0x01 == timestamp, rest is first 4 bytes
0x01 0x9A 0xBC 0xDE 0xF0		// 0x01 == timestamp (continued), rest is last 4 bytes
0x02 0x00 0x00 0x00 0x03		// 0x02 == DeviceType, 0x00000003 == thermostat
etc...

We could optimize, but out of scope for this library, its how someone can decide to use it. Its relevant for our calculations though, to estimate useage.
- Timestamp: 2026-05-12 10.30.00
- DevInfo: Thermostat, id 5, TemperatureChanged		(each of this 1 byte)
- Measurement: 18.9 C

We need some way if indicating the first record or the last record or both.
We need some crc, in case of powerloss.

Option A: One byte in every field.
Option B: Each record gets one header field, with some crc, perhaps some other info. 
Option C: Only with static record sizes. Reserve x bytes per record for this.

Note for later, i want some fast lookup. For example, filtering by device type or logcode. Perhaps some index for specific field keys. Perhaps we can add a field per record for x positions to next record with same field key? This is too much for me to thing about now, so ill park it here. But it probably has implications on the headers of sectors later. 


A little note about overwriting. Overwriting is from a user perspective. lets say he wants to track if a record has been handled he can:

log.Create()
   .Field(timestamp, xx)
   .Field(LogCode, TempChanged)
   .Field(Measurement, 18.4f)
   .Field(Actions, 0xFF)    // Note to self, need some way to ensure the user cant forget this field. Perhaps it needs to be part of the library. Some config or something that automatically appends this field.


Later somewhere in code:

for(iterate over log)
{
    if(logitem.getField(Actions) & 0x01) // means not handled
    {
        DoAction(logitem);
        log.Overwrite(logitem); 
    }
}


Ofcourse this needs something with an index. Something that makes it invalid in case circlebufferlog was overwritten. Things like that. Im thinking an indexer, that has some valid state. Im thinking storing a crc, then comparing that when overwriting if thats possible. So logitem in the code above could be an indexer.


idx = log.begin
while(idx.next(timeout)) // So it works with freertos? Preferably agnostic.
{
    flags = log.readField(idx, Actions);
    bla bla
    flags &= 0xF8;
    log.writeField(idx, Actions, flags);
}


In order to figure out, dynamic or static and the 3 options we have. Lets do some comparisons on how efficiently we use our flash.

Compare: 
- 1 byte keys, 4 byte values
- 1 byte keys, 8 byte values
- average of 3 records vs 5 records vs 7 records.

---

Did some numbers, Seems dynamic and option B is the most efficient on average.
Drawback is, we cant use numbers for indexes, we have to stick with iterators etc.

---

So, how do we recogninze the header field? We could reserve a key, less prefered since it impacts the user experience.

But thats not fair, a assumes only 1 byte header. B assumes 4 bytes header.
We may need both though, since we need to indiucate the start of a record. 

Also, We need to beable to 'deactivate' fields. Lets say a records spans 2 sectors. Clearing the first sector leaves some dangling fields in the second sector. If we completely write the first sector, it looks like the dangling ones are part of the last record in the fist sector. 

We could claim key 0x00 for the library.
- We can use the next byte for, what it is. Like 0xFF means emtpy. first bit low == written all bits low == erased. Etc...

Or we simply put one byte per field for housekeeping. this could also indicate system fields. 

So a field becomes
[HouseKeeping] [Key] [Value]

Would be great to have a hard seperation here. So first we build the fields system. Then we make the records system on top of that. Then the record system could also use multiple fields in case the user chooses too small value size for the record to store all its things in a single field.

Also fields are fixed size, so we can work with indexes. 

This would be cool, for indexes too. So if we write a record, we can say, in x places there is new information about this index chain. That new place can contain a pointer field. 

Not completely sure if we still want the housekeeping one in the field system. Or if that becomes part of the record system. The record system can tell the field system key size is what the user requests + 1. Or the record system can claim key 0x00 for itself. 

This way we also dont solve all issues in one go.


-----
Lets focus on fields first.
We need:
- Append
- Circle buffer
- Can never span multiple sectors!

It manages raw access to the flash and hides away any caluclations and headers in sectors for example.

We need to know if the flash partition is formatted for our usecase. 
We need to store the key and value sizes. (Do we really? We need an header anyway so might as well)
We could do something with indexes here, so we can quickly find the next or previous field with the same key?
- Each field can have a byte for double linked list in + or - x positions. Requires a bit of effort when writing. Also if a key is not written often we have to write dummy fields. 
- Cool idea, not sure yet. CAn also solve this in a level higher. 
- If keys are 1 byte, we could reserve in the header of a sector 256 bytes for lookup. Then again, percentually this is large if a sector is 4k. 
- Maybe this is too many tradeoff's now. Lets not do this now. Can revisit this idea later.

For now we only need key value pairs and some header. To make the appending and clearing field bits and circular behaviour of the fields. 

Maybe we should start to lock this in and start working on the code for fields first and see how things will fall into place later.

Also, dont build what we dont need. We can add it if it becomes an issue. So for now no housekeeping byte.



Info from other places:
- if CRC is used, we can't change the log entry afterwards
- Reserved keys instead of a housekeeping byte: 0xFF empty, 0x00 erased. (Can still be used for fields, but should not be in fields part of the code, can be in the records part of the code.) 
- Timestamp key marks the start of a record — no separate header field needed, the reserved key IS the marker. Fun but do we really want to force the user to always write this? We can also use the header for other thigns. Perhaps not such a good idea.
- Binary search, is this possible? We need something that is ordered. Perhaps this is something for later
- Indexer also for later, dont advance automatically, this gives unclear behaviour. Just mark it invalid or something.





Next:
C or C++
FreeRTOS or agnostic
It needs to be threadsafe, so can we do it without freertos?
The iterator for reading, do we want a wait untill unblocked, with some semaphore?

Im thinking, agnostic is cool and all, but i use esp idf primarely. I dont want to wire in all kind of crappy things. We could use my freertos wrapper things. Those could contain all freertos dependencies, and therefore be easely migratable to another system. those are C++, so we are bound to that then.

Maybe we dont need all this for just the field layer. 


----

Ok, we can determine key and value size via:

FieldStore store(flash);
store.format(1, 4);    // writes header: key=1B, value=4B
store.init();          // reads header, learns sizes from flash

---


Read Write takes index. Write should check if it was ok. Not sure if flash throws a fit if we set bits to 1, or if it just says ok and in reality its not ok. 

Eventually we need circulair behaviour and append. We also need to find the begin and end in that case. Not sure if its part of the field layer or if this is for later























