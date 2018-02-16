.PHONY: build run hubrun

build:
	docker build . -f Dockerfile -t mash:dev

run:
	docker run -it --rm \
		--mount source=mash-workspace,target=/workspace \
		mash:dev

hubrun:
	docker run -it --rm \
		--mount source=mash-workspace,target=/workspace \
		biswadipmaity/mash:latest