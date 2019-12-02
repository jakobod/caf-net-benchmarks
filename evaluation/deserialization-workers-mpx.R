library(tikzDevice)
library(ggplot2)
require(RColorBrewer)
require(gridExtra)

source("evaluation/human_readable.R")

net_zero_workers_data <- read.csv("evaluation/out/0serializer-mpx--x-deserializer.csv", sep=",", as.is=c(numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric))
net_zero_workers_data$avg <- rowMeans(net_zero_workers_data[,3:12])
net_zero_workers_data$sdev <- apply(net_zero_workers_data[,3:12], 1, sd)
net_zero_workers_data$proto <- '0 workers (net)'

io_data <- read.csv("evaluation/out/io-x-deserializer.csv", sep=",", as.is=c(numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric))
io_data$avg <- rowMeans(io_data[,3:12])
io_data$sdev <- apply(io_data[,3:12], 1, sd)
io_data$proto <- '0 workers (io)'

ppdf <- rbind(net_zero_workers_data)
ppdf <- rbind(ppdf, io_data)
ppdf$upper <- ppdf$avg + ppdf$sdev
ppdf$lower <- ppdf$avg - ppdf$sdev


pp_plot <- ggplot(ppdf, aes(x=deserializer, y=avg, color=proto)) +
  geom_line() + # size=0.8) +
  geom_point(aes(shape=proto), size = 2, stroke=0.8) +
  geom_errorbar(
    mapping=aes(
      ymin=lower,
      ymax=upper
    ),
    width=0.2
  ) +
  scale_shape_manual(values=c(1, 4, 3)) +
  scale_x_continuous(breaks=seq(0, 5, 1)) + # expand=c(0, 0), limits=c(0, 10)
  scale_y_continuous(labels = human_numbers, limits=c(55000000, 90000000), breaks=seq(40000000, 150000000, 10000000)) + # expand=c(0, 0), limits=c(0, 10)
  theme_bw() +
  theme(
    legend.title=element_blank(),
    #legend.text = element_text(size = 5),
    legend.key=element_rect(fill='white'), 
    legend.background=element_rect(fill="white", colour="black", size=0.25),
    legend.direction="vertical",
    legend.justification=c(0, 1),
    legend.position=c(0,1),
    # legend box background color
    legend.box.margin=margin(c(3, 3, 3, 3)),
    legend.key.height=unit(0.4,"line"),
    legend.key.size=unit(0.6, 'lines'),
    text=element_text(size=9),
    strip.text.x=element_blank()
  ) +
  # scale_color_grey() +
  scale_color_brewer(type="qual", palette=6) +
  #scale_color_manual(values=c('#4daf4a', '#e41a1c', '#377eb8')) +
  ggtitle("Variable deserializing workers") +
  labs(x="deserializing workers [#]", y="throughput [msg/s]")

tikz(file="figs/deserializing_workers-mpx.tikz", sanitize=TRUE, width=3.4, height=2.3)
pp_plot
dev.off()

ggsave("figs/deserializing_workers-mpx.pdf", plot=pp_plot, width=5, height=3)
