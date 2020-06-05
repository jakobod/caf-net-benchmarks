library(tikzDevice)
library(ggplot2)
require(RColorBrewer)
require(gridExtra)

source("evaluation/human_readable.R")

pingpong_net_master <- read.csv("evaluation/data/pingpong-multiple-1-pings-alloc-net.out", sep=",", as.is=c(numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric))
pingpong_net_master$avg <- rowMeans(pingpong_net_master[,2:11])
pingpong_net_master$sdev <- apply(pingpong_net_master[,2:11], 1, sd)
pingpong_net_master$proto <- '1 net - master'

pingpong_io <- read.csv("evaluation/data/pingpong-multiple-1-pings-alloc-io.out", sep=",", as.is=c(numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric))
pingpong_io$avg <- rowMeans(pingpong_io[,2:11])
pingpong_io$sdev <- apply(pingpong_io[,2:11], 1, sd)
pingpong_io$proto <- '2 - io'

ppdf <- rbind(pingpong_net_master, pingpong_io)
ppdf$upper <- ppdf$avg + ppdf$sdev
ppdf$lower <- ppdf$avg - ppdf$sdev


pp_plot <- ggplot(ppdf, aes(x=num_pings, y=avg, color=proto)) +
  geom_line() + # size=0.8) +
  geom_point(aes(shape=proto), size = 1, stroke=0.8) +
  geom_errorbar(
    mapping=aes(
      ymin=lower,
      ymax=upper
    ),
    width=0.5
  ) +
  scale_x_continuous(breaks=seq(1, 32, 1)) + # expand=c(0, 0), limits=c(0, 10)
  scale_y_continuous(labels = human_numbers, limits=c(0,2600000), breaks=seq(0, 2500000, 250000)) + # expand=c(0, 0), limits=c(0, 10)
  theme_bw() +
  theme(
    legend.title=element_blank(),
    legend.key=element_rect(fill='white'), 
    legend.background=element_rect(fill="white", colour="black", size=0.25),
    legend.direction="vertical",
    legend.justification=c(0, 1),
    legend.position=c(0,1),
    legend.box.margin=margin(c(3, 3, 3, 3)),
    legend.key.height=unit(0.4,"line"),
    legend.key.size=unit(0.6, 'lines'),
    text=element_text(size=9),
    strip.text.x=element_blank()
  ) +
  scale_color_brewer(type="qual", palette=6) +
  ggtitle("Pingpong multiple remote nodes 1 Ping") +
  labs(x="remote nodes [#]", y="allocations [alloc/s]")

tikz(file="figs/pingpong-multiple-1-alloc.tikz", sanitize=TRUE, width=3.4, height=2.3)
pp_plot
dev.off()

ggsave("figs/pingpong-multiple-1-alloc.pdf", plot=pp_plot, width=8, height=5)

