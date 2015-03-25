
Architecture
------------

Simplified hierarchial architecture
                                                    
							 +-----------+                          
							 | Directory |                          
							 | Server D  |                         
							 +-----------+                          
							 /            \                           
							/              \                          
						 /                \                         
		+---------------+          +---------------+                       
		| File Server A | <------> | File Server B |                     
		+---------------+          +---------------+ 
					/        \                 /       \
		+----------+ +----------+  +----------+ +----------+                   
		| Client 1 | | Client 2 |  | Client 3 | | Client 4 |
		+----------+ +----------+  +----------+ +----------+                           
                                                   
Every client uses only one file server to access files at a time. All file servers use the centralized directory. File servers communicate with each other to excange files.

<center><img alt="System architecture diagram" src="arch.svg" width="75%"></center>

