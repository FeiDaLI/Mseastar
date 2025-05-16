
#!/bin/bash
PORTS=(10000 41234 41235 41236 41237)
for PORT in "${PORTS[@]}"; do
    PIDS=$(lsof -ti :$PORT)
    if [ -z "$PIDS" ]; then
        echo "$PORT is free"
    else
        kill -9 $PIDS
        if [ $? -eq 0 ]; then
            echo "kill $PORT "
        else
            echo "kill fail "
        fi
    fi
done