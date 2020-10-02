library(tikzDevice)
library(ggplot2)
require(RColorBrewer)
require(gridExtra)

source("evaluation/human_readable.R")

timing_net <- read.csv("evaluation/data/timing-net.out", sep=",")[ ,1:10000]
timing_net$avg <- rowMeans(timing_net[,2:10000])
timing_net$sdev <- apply(timing_net[ ,2:10000], 1, sd)
timing_net$backend <- 'libcaf_net'

timing_io <- read.csv("evaluation/data/timing-io.out", sep=",")[ ,1:10000]
timing_io$avg <- rowMeans(timing_io[,2:10000])
timing_io$sdev <- apply(timing_io[ ,2:10000], 1, sd)
timing_io$backend <- 'libcaf_io'

timing_raw <- read.csv("evaluation/data/timing-raw.out", sep=",")[ ,1:10000]
timing_raw$avg <- rowMeans(timing_raw[,2:10000])
timing_raw$sdev <- apply(timing_raw[ ,2:10000], 1, sd)
timing_raw$backend <- 'raw sockets'

ppdf <- rbind(timing_net, timing_io, timing_raw)
ppdf$upper <- ppdf$avg + ppdf$sdev
ppdf$lower <- ppdf$avg - ppdf$sdev

pp_plot <- ggplot(ppdf, aes(x=what, y=avg, fill=backend)) + 
  scale_y_continuous(labels = human_numbers, limits=c(0,130), breaks=seq(0, 130, 10)) +
  geom_bar(stat="identity", position=position_dodge()) +
  geom_errorbar(aes(ymin=lower, ymax=upper), width=.2,
                position=position_dodge(.9))+
  ggtitle("Durations in Pingpong") +
  scale_fill_brewer(palette="Dark2") +
  labs(x="Interval", y="duration [µs]")

# tikz(file="figs/pingpong-timings.tikz", sanitize=TRUE, width=3.4, height=2.3)
pp_plot
dev.off()

ggsave("figs/timings.pdf", plot=pp_plot, width=8, height=5)
