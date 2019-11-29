library(tikzDevice)
library(ggplot2)
require(RColorBrewer)
require(gridExtra)

source("evaluation/human_readable.R")

ppconserwork <- read.csv("evaluation/out/x-serializer-0-deserializer.csv", sep=",", as.is=c(numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric))
ppconserwork$avg <- rowMeans(ppconserwork[,3:12])
ppconserwork$sdev <- apply(ppconserwork[,3:12], 1, sd)
ppconserwork$proto <- '0-deserializing workers'

ppcondeserwork <- read.csv("evaluation/out/x-serializer-4-deserializer.csv", sep=",", as.is=c(numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric, numeric))
ppcondeserwork$avg <- rowMeans(ppcondeserwork[,3:12])
ppcondeserwork$sdev <- apply(ppcondeserwork[,3:12], 1, sd)
ppcondeserwork$proto <- '4-deserializing workers'

ppdf <- rbind(ppconserwork)
ppdf <- rbind(ppdf, ppcondeserwork)
ppdf$upper <- ppdf$avg + ppdf$sdev
ppdf$lower <- ppdf$avg - ppdf$sdev


pp_plot <- ggplot(ppdf, aes(x=serializer, y=avg, color=proto)) +
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
  scale_y_continuous(labels = human_numbers, limits=c(50000000, 120000000), breaks=seq(40000000, 140000000, 10000000)) + # expand=c(0, 0), limits=c(0, 10)
  theme_bw() +
  theme(
    legend.title=element_blank(),
    legend.key=element_rect(fill='white'), 
    legend.background=element_rect(fill="white", colour="black", size=0.25),
    legend.direction="vertical",
    legend.justification=c(0, 1),
    legend.position=c(0,1),
    # legend box background color
    legend.box.margin=margin(c(3, 3, 3, 3)),
    legend.key.size=unit(0.8, 'lines'),
    text=element_text(size=9),
    strip.text.x=element_blank()
  ) +
  # scale_color_grey() +
  scale_color_brewer(type="qual", palette=6) +
  ggtitle("Variable serializing workers") +
  labs(x="workers [#]", y="throughput [msg/s]")

tikz(file="figs/serializing_workers.tikz", sanitize=TRUE, width=3.4, height=2.3)
pp_plot
dev.off()

ggsave("figs/serializing_workers.pdf", plot=pp_plot, width=3.4, height=2.3)

