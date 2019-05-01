#!/bin/bash
rm -rf nohup.out
./beringei/service/beringei_main \
	    -beringei_configuration_path /tmp/beringei.json \
	        -create_directories \
		    -sleep_between_bucket_finalization_secs 60 \
		        -allowed_timestamp_behind 3600 -allowed_timestamp_ahead 3600\
			    -bucket_size 7200 \
			        -buckets $((86400/7200)) \
				    -logtostderr \
				        -v=2