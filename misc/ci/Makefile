# Taisei Docker Makefile

tag:
ifndef TAG
	@echo "tag not defined, please define one"
	@echo "i.e: TAG=20210630 make docker/switch"
	@exit 1
endif

##########
# docker #
##########

docker/windows: tag
	docker build . -m 8GB -t taiseiproject/windows-toolkit:$(TAG) -f "Dockerfile.windows"
	docker push taisei/windows-toolkit:$(TAG)

docker/switch: tag
	docker build . -m 4GB -t taiseiproject/switch-toolkit:$(TAG) -f "Dockerfile.switch"
	docker push taiseiproject/switch-toolkit:$(TAG)

docker/linux: tag
	docker build . -m 4GB -t taiseiproject/linux-toolkit:$(TAG) -f "Dockerfile.linux"
	docker push taiseiproject/linux-toolkit:$(TAG)
