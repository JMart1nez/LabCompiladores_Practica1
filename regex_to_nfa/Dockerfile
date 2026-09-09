# Use the latest Alpine image as the base
FROM alpine:latest
# Install build tools (gcc , musl -dev , make , cmake ) for compiling the code
RUN apk add --no-cache gcc g++ musl-dev make cmake
# Create a directory for the application
WORKDIR /app
# Copy the source code into the container
COPY . .
# Container starts by compiling the code and running the validator
CMD ["sh", "-c", "cmake -B build -S . && cmake --build build && ./build/validator"]